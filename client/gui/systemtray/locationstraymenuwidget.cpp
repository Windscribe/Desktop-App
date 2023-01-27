#include "locationstraymenuwidget.h"

#include <QPainter>
#include <QTimer>
#include <QEvent>
#include <QApplication>
#include <QVBoxLayout>
#include <QScreen>
#include "commongraphics/commongraphics.h"
#include "locationstraymenuscalemanager.h"

#ifdef Q_OS_MAC
#include "utils/widgetutils_mac.h"
#endif

LocationsTrayMenuWidget::LocationsTrayMenuWidget(LocationsTrayMenuType type, QWidget *parent) :
    QWidget(parent)
  , locationType_(type)
  , bIsFreeSession_(false)
  , currentSubmenu_(nullptr)
  , visibleItemsCount_(20)
{
    upButton_ = new LocationsTrayMenuButton();
    listWidget_ = new QListWidget();
    listWidget_->setMouseTracking(true);
    listWidget_->setStyleSheet("background-color: rgba(255, 255, 255, 0);\nborder-top: none;\nborder-bottom: none;");
    listWidget_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget_->setResizeMode(QListView::Adjust);
    connect(listWidget_, SIGNAL(itemClicked(QListWidgetItem*)),
            SLOT(onListWidgetItemTriggered(QListWidgetItem*)));

    downButton_ = new LocationsTrayMenuButton();
    downButton_->setType(1);
    connect(upButton_, SIGNAL(clicked()), SLOT(onScrollUpClick()));
    connect(downButton_, SIGNAL(clicked()), SLOT(onScrollDownClick()));

    QVBoxLayout *widgetLayout = new QVBoxLayout();
    widgetLayout->addWidget(upButton_);
    widgetLayout->addWidget(listWidget_);
    widgetLayout->addWidget(downButton_);
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(0);
    setLayout(widgetLayout);

    recalcSize();

    locationsTrayMenuItemDelegate_ = new LocationsTrayMenuItemDelegate(this);
    listWidget_->setItemDelegate(locationsTrayMenuItemDelegate_);
    listWidget_->viewport()->installEventFilter(this);
}

LocationsTrayMenuWidget::~LocationsTrayMenuWidget()
{

}

bool LocationsTrayMenuWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == listWidget_->viewport()) {
        switch (event->type()) {
        case QEvent::Wheel:
            handleMouseWheel();
            break;
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
            handleMouseMove();
            break;
        case QEvent::Hide:
            if (currentSubmenu_) {
                currentSubmenu_->close();
                currentSubmenu_ = nullptr;
            }
            listWidget_->clearSelection();
            break;
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void LocationsTrayMenuWidget::handleMouseMove()
{
    updateTableViewSelection();
    updateSubmenuForSelection();
}

void LocationsTrayMenuWidget::handleMouseWheel()
{
    if (currentSubmenu_) {
        currentSubmenu_->close();
        currentSubmenu_ = nullptr;
    }
    QTimer::singleShot(1, this, SLOT(updateTableViewSelection()));
    updateBackground_mac();
}

void LocationsTrayMenuWidget::setLocationsModel(LocationsModel *locationsModel)
{
    connect(locationsModel->getAllLocationsModel(), SIGNAL(itemsUpdated(QVector<LocationModelItem*>)), SLOT(onItemsUpdated(QVector<LocationModelItem *>)));
    connect(locationsModel->getAllLocationsModel(), SIGNAL(connectionSpeedChanged(LocationID,PingTime)), SLOT(onConnectionSpeedChanged(LocationID,PingTime)));
    connect(locationsModel->getAllLocationsModel(), SIGNAL(freeSessionStatusChanged(bool)), SLOT(onSessionStatusChanged(bool)));
    connect(locationsModel->getFavoriteLocationsModel(), SIGNAL(itemsUpdated(QVector<CityModelItem*>)), SLOT(onFavoritesUpdated(QVector<CityModelItem *>)));
    connect(locationsModel->getStaticIpsLocationsModel(), SIGNAL(itemsUpdated(QVector<CityModelItem*>)), SLOT(onStaticIpsUpdated(QVector<CityModelItem *>)));
    connect(locationsModel->getConfiguredLocationsModel(), SIGNAL(itemsUpdated(QVector<CityModelItem*>)), SLOT(onCustomConfigsUpdated(QVector<CityModelItem *>)));
}

void LocationsTrayMenuWidget::setFontForItems(const QFont &font)
{
    locationsTrayMenuItemDelegate_->setFontForItems(font);
    recalcSize();
    updateShortenedTexts();
}

void LocationsTrayMenuWidget::onListWidgetItemTriggered(QListWidgetItem *item)
{
    Q_ASSERT(item);
   const bool isEnabled = item->data(USER_ROLE_FLAGS).toInt() & ITEM_FLAG_IS_ENABLED;
   if (!isEnabled)
       return;
   // Ignore if there is a submenu.
    auto it = menuMap_.find(item);
    if (it != menuMap_.end())
        return;
    emit locationSelected(locationType_, item->data(USER_ROLE_TITLE).toString(), -1);
}

void LocationsTrayMenuWidget::onSubmenuActionTriggered(QAction *action)
{
    Q_ASSERT(currentSubmenu_ && action);
    if (!currentSubmenu_ || !action)
        return;
    QModelIndex ind = listWidget_->currentIndex();
    if (!ind.isValid())
        return;
    QListWidgetItem *item = listWidget_->item(ind.row());
    bool bIsSelectable = item->data(USER_ROLE_FLAGS).toInt() & ITEM_FLAG_IS_ENABLED;
    if (!bIsSelectable)
        return;
    emit locationSelected(locationType_, item->data(USER_ROLE_TITLE).toString(),
                          currentSubmenu_->actions().indexOf(action));
}

void LocationsTrayMenuWidget::updateTableViewSelection()
{
    QPoint pt = listWidget_->viewport()->mapFromGlobal(QCursor::pos());
    QModelIndex ind = listWidget_->indexAt(pt);
    if (ind.isValid() && ind.column() == 0)
    {
        listWidget_->setCurrentIndex(ind);
    }
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onScrollUpClick()
{
    int ind = listWidget_->currentRow();
    if (ind > 0)
    {
        listWidget_->setCurrentRow(ind - 1);
    }
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onScrollDownClick()
{
    int ind = listWidget_->currentRow();
    if (ind < (listWidget_->count() - 1))
    {
        listWidget_->setCurrentRow(ind + 1);
    }
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::clearItems()
{
    map_.clear();
    listWidget_->clear();

    qDeleteAll(menuMap_);
    menuMap_.clear();

    downButton_->setVisible(false);
    upButton_->setVisible(false);
}

void LocationsTrayMenuWidget::recalcSize()
{
    QStyleOptionMenuItem opt;
    QSize sz;
    sz = QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz);
    const int scaledItemHeight = sz.height() * LocationsTrayMenuScaleManager::instance().scale();
    visibleItemsCount_ = listWidget_->count();
    if (visibleItemsCount_ < 1)
        visibleItemsCount_ = 1;

    QScreen *screen = LocationsTrayMenuScaleManager::instance().screen();
    if (screen) {
        const int availableHeight = screen->geometry().height() -
            upButton_->sizeHint().height() - downButton_->sizeHint().height();
        const int maxItemCount = availableHeight / scaledItemHeight - 1;
        if (visibleItemsCount_ > maxItemCount)
            visibleItemsCount_ = maxItemCount;
    }

    int maxWidth = 190 * LocationsTrayMenuScaleManager::instance().scale(); // set initial mimimum size
    // if the size of any item exceeds maxWidth, then will increase maxWidth
    for (int i = 0; i < listWidget_->count(); ++i)
    {
        QListWidgetItem *item = listWidget_->item(i);
        int width = locationsTrayMenuItemDelegate_->calcWidth(item->text(), item->data(LocationsTrayMenuWidget::USER_ROLE_COUNTRY_CODE).toString(),
                                                  item->data(LocationsTrayMenuWidget::USER_ROLE_FLAGS).toInt());
        if (width > maxWidth)
        {
            maxWidth = width;
        }
    }

    listWidget_->setFixedSize(maxWidth, scaledItemHeight * visibleItemsCount_);
}

void LocationsTrayMenuWidget::updateShortenedTexts()
{
    // Only custom config names are shortened, for now.
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_CUSTOM_CONFIGS)
        return;

    for (int ind = 0; ind < listWidget_->count(); ++ind) {
        QListWidgetItem *item = listWidget_->item(ind);
        QString itemName = CommonGraphics::truncatedText(
            item->data(USER_ROLE_ORIGINAL_NAME).toString(),
            locationsTrayMenuItemDelegate_->getFontForItems(),
            listWidget_->width()- 30 * LocationsTrayMenuScaleManager::instance().scale());
        item->setText(itemName);
    }
}

void LocationsTrayMenuWidget::onItemsUpdated(QVector<LocationModelItem *> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_GENERIC)
        return;

    clearItems();

    for (const LocationModelItem *item: qAsConst(items))
    {
        if (item->id.isCustomConfigsLocation())
        {
            continue;
        }

        QString itemName = item->title;
        const QVector<CityModelItem> cities = item->cities;

        if (cities.count() > 0)
        {
            bool containsAtLeastOneNonProCity = false;
            for (const CityModelItem &city: cities)
            {
                if (!city.bShowPremiumStarOnly)
                {
                    containsAtLeastOneNonProCity = true;
                    break;
                }
            }

            if (!containsAtLeastOneNonProCity && bIsFreeSession_)
            {
                itemName += " (Pro)";
            }

            QListWidgetItem *listItem = new QListWidgetItem(itemName);
            listWidget_->addItem(listItem);

            int itemFlags = ITEM_FLAG_IS_VALID | ITEM_FLAG_HAS_SUBMENU;
            if (containsAtLeastOneNonProCity || !bIsFreeSession_)
            {
                const auto connectionSpeed = PingTime(item->calcAveragePing()).toConnectionSpeed();
                if (connectionSpeed != 0)
                    itemFlags |= ITEM_FLAG_IS_ENABLED;
            }
            if (!containsAtLeastOneNonProCity)
                itemFlags |= ITEM_FLAG_IS_PREMIUM_ONLY;
            if (!item->countryCode.isEmpty())
                itemFlags |= ITEM_FLAG_HAS_COUNTRY;

            listItem->setData(USER_ROLE_FLAGS, itemFlags);
            listItem->setData(USER_ROLE_TITLE, item->title);
            listItem->setData(USER_ROLE_ORIGINAL_NAME, item->title);
            listItem->setData(USER_ROLE_COUNTRY_CODE, item->countryCode);

            auto *submenu = new LocationsTrayMenuWidgetSubmenu(this);
            QVector<bool> cityProInfo(cities.count());
            for (int i = 0; i < cities.count(); ++i) {
                const auto &city = cities[i];
                QString cityName = city.makeTitle();
                QString visibleName = cityName;
                cityProInfo[i] = city.bShowPremiumStarOnly;
                bool isEnabled = !city.bShowPremiumStarOnly || !bIsFreeSession_;
                if (!isEnabled)
                    visibleName += " (Pro)";
                auto *action = submenu->addAction(visibleName);
                action->setObjectName(cityName);
                action->setEnabled(isEnabled);
            }
            connect(submenu, SIGNAL(triggered(QAction*)), SLOT(onSubmenuActionTriggered(QAction*)));
            listItem->setData(USER_ROLE_CITY_INFO, QVariant::fromValue(cityProInfo));

            map_[item->id] = listItem;
            menuMap_[listItem] = submenu;
        }
    }

    recalcSize();
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onFavoritesUpdated(QVector<CityModelItem*> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_FAVORITES)
        return;

    clearItems();

    for (const CityModelItem *item : qAsConst(items))
    {
        if (item->city.isEmpty())
            continue;

        QString itemTitle = item->makeTitle();
        QString itemName = itemTitle;
        int itemFlags = ITEM_FLAG_IS_VALID;
        if (item->bShowPremiumStarOnly && bIsFreeSession_) {
            itemName += " (Pro)";
            itemFlags |= ITEM_FLAG_IS_PREMIUM_ONLY;
        } else {
            if (item->pingTimeMs != 0)
                itemFlags |= ITEM_FLAG_IS_ENABLED;
        }
        if (!item->countryCode.isEmpty())
            itemFlags |= ITEM_FLAG_HAS_COUNTRY;

        QListWidgetItem *listItem = new QListWidgetItem(itemName);
        listWidget_->addItem(listItem);

        listItem->setData(USER_ROLE_FLAGS, itemFlags);
        listItem->setData(USER_ROLE_TITLE, itemTitle);
        listItem->setData(USER_ROLE_ORIGINAL_NAME, itemName);
        listItem->setData(USER_ROLE_COUNTRY_CODE, item->countryCode);

        map_[item->id] = listItem;
    }

    recalcSize();
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onStaticIpsUpdated(QVector<CityModelItem*> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_STATIC_IPS)
        return;

    clearItems();

    for (const CityModelItem *item : qAsConst(items))
    {
        if (item->staticIp.isEmpty())
            continue;

        const QString itemName = item->makeTitle();
        QListWidgetItem *listItem = new QListWidgetItem(itemName);
        listWidget_->addItem(listItem);

        int itemFlags = ITEM_FLAG_IS_ENABLED | ITEM_FLAG_IS_VALID;
        if (!item->countryCode.isEmpty())
            itemFlags |= ITEM_FLAG_HAS_COUNTRY;

        listItem->setData(USER_ROLE_FLAGS, itemFlags);
        listItem->setData(USER_ROLE_TITLE, itemName);
        listItem->setData(USER_ROLE_ORIGINAL_NAME, itemName);
        listItem->setData(USER_ROLE_COUNTRY_CODE, item->countryCode);

        map_[item->id] = listItem;
    }

    recalcSize();
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onCustomConfigsUpdated(QVector<CityModelItem*> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_CUSTOM_CONFIGS)
        return;

    clearItems();

    for (const CityModelItem *item : qAsConst(items))
    {
        if (item->city.isEmpty())
            continue;

        const QString itemName = item->makeTitle();
        QListWidgetItem *listItem = new QListWidgetItem(itemName);
        listWidget_->addItem(listItem);

        int itemFlags = 0;
        if (!item->isDisabled && item->isCustomConfigCorrect)
            itemFlags |= ITEM_FLAG_IS_ENABLED | ITEM_FLAG_IS_VALID;

        listItem->setData(USER_ROLE_FLAGS, itemFlags);
        listItem->setData(USER_ROLE_TITLE, itemName);
        listItem->setData(USER_ROLE_ORIGINAL_NAME, itemName);

        map_[item->id] = listItem;
    }

    recalcSize();
    updateShortenedTexts();
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onSessionStatusChanged(bool bFreeSessionStatus)
{
    bIsFreeSession_ = bFreeSessionStatus;

    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_GENERIC &&
        locationType_ != LOCATIONS_TRAY_MENU_TYPE_FAVORITES)
        return;

    for (int ind = 0; ind < listWidget_->count(); ++ind)
    {
        QListWidgetItem *item = listWidget_->item(ind);

        QString itemName = item->data(USER_ROLE_ORIGINAL_NAME).toString();
        int flags = item->data(USER_ROLE_FLAGS).toInt();

        if ((flags & ITEM_FLAG_IS_PREMIUM_ONLY) && bIsFreeSession_) {
            itemName += " (Pro)";
            flags &= ~ITEM_FLAG_IS_ENABLED;
        } else if (flags & ITEM_FLAG_IS_VALID) {
            flags |= ITEM_FLAG_IS_ENABLED;
        }
        item->setText(itemName);
        item->setData(USER_ROLE_FLAGS, flags);

        auto *submenu = menuMap_[item];
        if (submenu) {
            const auto cityProInfo = item->data(USER_ROLE_CITY_INFO).value<QVector<bool>>();
            for (int i = 0; i < submenu->actions().count(); ++i) {
                auto *action = submenu->actions().at(i);
                QString visibleName = action->objectName();
                bool isEnabled = !cityProInfo[i] || !bIsFreeSession_;
                if (!isEnabled)
                    visibleName += " (Pro)";
                action->setText(visibleName);
                action->setEnabled(isEnabled);
            }
        }
    }
}

void LocationsTrayMenuWidget::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_GENERIC &&
        locationType_ != LOCATIONS_TRAY_MENU_TYPE_FAVORITES)
        return;

    auto it = map_.find(id);
    if (it != map_.end())
    {
        QListWidgetItem *item = it.value();
        int flags = item->data(USER_ROLE_FLAGS).toInt();
        if ((flags & ITEM_FLAG_IS_VALID) && timeMs.toConnectionSpeed() != 0)
            flags |= ITEM_FLAG_IS_ENABLED;
        else
            flags &= ~ITEM_FLAG_IS_ENABLED;
        item->setData(USER_ROLE_FLAGS, flags);
    }
}

void LocationsTrayMenuWidget::updateSubmenuForSelection()
{
    LocationsTrayMenuWidgetSubmenu *submenu = nullptr;
    QPoint popupPosition;
    QModelIndex ind = listWidget_->currentIndex();
    if (ind.isValid()) {
        const QListWidgetItem *item = listWidget_->item(ind.row());
        const bool isEnabled = item->data(USER_ROLE_FLAGS).toInt() & ITEM_FLAG_IS_ENABLED;
        if (isEnabled) {
            auto it = menuMap_.find(item);
            if (it != menuMap_.end())
                submenu = it.value();
        }
        const auto rc = listWidget_->visualItemRect(item);
        popupPosition = listWidget_->viewport()->mapToGlobal(rc.topRight());
    }
    if (currentSubmenu_ != submenu) {
        if (currentSubmenu_)
            currentSubmenu_->close();
        if (submenu) {
            submenu->popup(popupPosition);
        }
        currentSubmenu_ = submenu;
    }
    else if (currentSubmenu_ && !currentSubmenu_->isVisible()) {
        currentSubmenu_->popup(popupPosition);
    }
}

void LocationsTrayMenuWidget::updateButtonsState()
{
    QModelIndex ind = listWidget_->indexAt(QPoint(2,2));
    if (ind.isValid())
    {
        upButton_->setEnabled(ind.row() != 0);
        downButton_->setEnabled((ind.row() + visibleItemsCount_) < (listWidget_->count()));
    }

    const bool kAllowScrolling = listWidget_->count() > visibleItemsCount_;
    upButton_->setVisible(kAllowScrolling);
    downButton_->setVisible(kAllowScrolling);
}

void LocationsTrayMenuWidget::updateBackground_mac()
{
#ifdef Q_OS_MAC
    // This is a hack to force repaint of the window background. For some reason, Qt doesn't do that
    // even when this is essential, e.g. before painting a widget with a transparent background,
    // such as |listWidget_|. It seems that such behaviour is OSX version dependent. This call
    // invalidates the whole submenu window to ensure the translucent menu background is repainted.
    WidgetUtils_mac::setNeedsDisplayForWindow(this);
#endif
}

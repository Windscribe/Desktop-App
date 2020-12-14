#include "locationstraymenuwidget.h"

#include <QPainter>
#include <QTimer>
#include <QEvent>
#include <QApplication>
#include <QVBoxLayout>
#include <QDebug>
#include "dpiscalemanager.h"

#ifdef Q_OS_MAC
#include "utils/widgetutils_mac.h"
#endif

LocationsTrayMenuWidget::LocationsTrayMenuWidget(QWidget *parent) :
    QWidget(parent)
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
    listWidget_->setResizeMode(QListView::Fixed);
    downButton_ = new LocationsTrayMenuButton();
    downButton_->setType(1);
    connect(upButton_, SIGNAL(clicked()), SLOT(onScrollUpClick()));
    connect(downButton_, SIGNAL(clicked()), SLOT(onScrollDownClick()));

    QVBoxLayout *widgetLayout = new QVBoxLayout();
    widgetLayout->addWidget(upButton_);
    widgetLayout->addWidget(listWidget_);
    widgetLayout->addWidget(downButton_);
    widgetLayout->setMargin(0);
    widgetLayout->setSpacing(0);
    setLayout(widgetLayout);

    QStyleOptionMenuItem opt;
    QSize sz;
    sz = QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz);
    const int scaledItemHeight = sz.height() * G_SCALE;

    const auto *screen = qApp->primaryScreen();
    if (screen) {
        const int availableHeight = screen->geometry().height() -
            upButton_->sizeHint().height() - downButton_->sizeHint().height();
        visibleItemsCount_ = availableHeight / scaledItemHeight - 1;
    }
    listWidget_->setFixedSize(190 * G_SCALE, scaledItemHeight * visibleItemsCount_);

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
}

void LocationsTrayMenuWidget::setFontForItems(const QFont &font)
{
    locationsTrayMenuItemDelegate_->setFontForItems(font);
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
    bool bIsSelectable = item->data(USER_ROLE_ENABLED).toBool();
    if (!bIsSelectable)
        return;
    emit locationSelected(item->data(USER_ROLE_TITLE).toString(),
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

void LocationsTrayMenuWidget::onItemsUpdated(QVector<LocationModelItem *> items)
{
    map_.clear();
    listWidget_->clear();

    qDeleteAll(menuMap_);
    menuMap_.clear();

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

            if (containsAtLeastOneNonProCity || !bIsFreeSession_)
            {
                const auto connectionSpeed = PingTime(item->calcAveragePing()).toConnectionSpeed();
                listItem->setData(USER_ROLE_ENABLED, connectionSpeed != 0);
            }
            else
            {
                listItem->setData(USER_ROLE_ENABLED, false);
            }
            listItem->setData(USER_ROLE_TITLE, item->title);
            listItem->setData(USER_ROLE_ORIGINAL_NAME, item->title);
            listItem->setData(USER_ROLE_IS_PREMIUM_ONLY, !containsAtLeastOneNonProCity);
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

    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onSessionStatusChanged(bool bFreeSessionStatus)
{
    bIsFreeSession_ = bFreeSessionStatus;

    for (int ind = 0; ind < listWidget_->count(); ++ind)
    {
        QListWidgetItem *item = listWidget_->item(ind);

        QString itemName = item->data(USER_ROLE_ORIGINAL_NAME).toString();
        if (item->data(USER_ROLE_IS_PREMIUM_ONLY).toBool() && bIsFreeSession_)
        {
            itemName += " (Pro)";
        }
        item->setText(itemName);

        if (item->data(USER_ROLE_IS_PREMIUM_ONLY).toBool() && bIsFreeSession_)
        {
            item->setData(USER_ROLE_ENABLED, false);
        }
        else
        {
            item->setData(USER_ROLE_ENABLED, true);
        }

        auto *submenu = menuMap_[item];
        const auto cityProInfo = item->data(USER_ROLE_CITY_INFO).value<QVector<bool>>();
        if (submenu) {
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
    auto it = map_.find(id);
    if (it != map_.end())
    {
        QListWidgetItem *item = it.value();
        item->setData(USER_ROLE_ENABLED, timeMs.toConnectionSpeed() != 0);
    }
}

void LocationsTrayMenuWidget::updateSubmenuForSelection()
{
    LocationsTrayMenuWidgetSubmenu *submenu = nullptr;
    QPoint popupPosition;
    QModelIndex ind = listWidget_->currentIndex();
    if (ind.isValid()) {
        const QListWidgetItem *item = listWidget_->item(ind.row());
        const bool isEnabled = item->data(USER_ROLE_ENABLED).toBool();
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

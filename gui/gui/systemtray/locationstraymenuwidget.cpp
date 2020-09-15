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
  , bIsFreeSession_(true)

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
    listWidget_->setFixedSize(155 * G_SCALE, sz.height()*VISIBLE_ITEMS_COUNT);

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
            QTimer::singleShot(1, this, SLOT(updateTableViewSelection()));
            updateBackground_mac();
            break;
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
            updateTableViewSelection();
            break;
        case  QEvent::MouseButtonRelease:
            {
                QPoint pt = listWidget_->viewport()->mapFromGlobal(QCursor::pos());
                QModelIndex ind = listWidget_->indexAt(pt);
                if (ind.isValid()) {
                    QListWidgetItem *item = listWidget_->item(ind.row());
                    bool bIsSelectable = item->data(USER_ROLE_ENABLED).toBool();
                    if (bIsSelectable)
                        emit locationSelected(item->data(USER_ROLE_ID).toInt());
                }
                break;
            }
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
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

    for (const LocationModelItem *item: qAsConst(items))
    {
        if (item->id.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
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
                }
            }

            if (!containsAtLeastOneNonProCity)
            {
                itemName += " (Pro)";
            }

            QListWidgetItem *listItem = new QListWidgetItem(itemName);
            listWidget_->addItem(listItem);

            if (containsAtLeastOneNonProCity)
            {
                listItem->setData(USER_ROLE_ENABLED, true /*item->pingTimeMs.toConnectionSpeed() != 0*/);  //todo
            }
            else
            {
                listItem->setData(USER_ROLE_ENABLED, false);
            }
            listItem->setData(USER_ROLE_ID, item->id.getId());
            listItem->setData(USER_ROLE_ORIGINAL_NAME, item->title);
            listItem->setData(USER_ROLE_IS_PREMIUM_ONLY, !containsAtLeastOneNonProCity);

            map_[item->id.getId()] = listItem;
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
    }
}

void LocationsTrayMenuWidget::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    QMap<int, QListWidgetItem *>::iterator it = map_.find(id.getId());
    if (it != map_.end())
    {
        QListWidgetItem *item = it.value();
        item->setData(USER_ROLE_ENABLED, timeMs.toConnectionSpeed() != 0);
    }
}

void LocationsTrayMenuWidget::updateButtonsState()
{
    QModelIndex ind = listWidget_->indexAt(QPoint(2,2));
    if (ind.isValid())
    {
        upButton_->setEnabled(ind.row() != 0);
        downButton_->setEnabled((ind.row() + VISIBLE_ITEMS_COUNT) < (listWidget_->count()));
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

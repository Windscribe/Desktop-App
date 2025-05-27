#include "locationstraymenuwidget.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVBoxLayout>
#include "commongraphics/commongraphics.h"
#include "locations/locationsmodel_roles.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_MACOS
#include "widgetutils/widgetutils_mac.h"
#endif

class LocationsTrayMenuWidget::LocationsTrayMenuWidgetSubmenu : public QMenu
{
public:
    explicit LocationsTrayMenuWidgetSubmenu(LocationsTrayMenuWidget *host)
        : QMenu(host), host_(host) {}

    void mouseMoveEvent(QMouseEvent *event) override
    {
        QMenu::mouseMoveEvent(event);
        if (!geometry().contains(QCursor::pos()))
            host_->handleMouseMove();
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            event->ignore();
            return;
        }
        QMenu::mousePressEvent(event);
    }
    void wheelEvent(QWheelEvent *event) override
    {
        QMenu::wheelEvent(event);
        if (!geometry().contains(QCursor::pos()))
            host_->handleMouseWheel();
    }
private:
    LocationsTrayMenuWidget *host_;
};

LocationsTrayMenuWidget::LocationsTrayMenuWidget(QWidget *parent, QAbstractItemModel *model, const QFont &font, const QRect &trayIconGeometry) :
    QWidget(parent)
  , currentSubmenu_(nullptr)
  , visibleItemsCount_(20)
{
    screen_ = qApp->screenAt(trayIconGeometry.topLeft());
    WS_ASSERT(screen_ != nullptr);
    if (!screen_)
        screen_ = qApp->primaryScreen();

    if (screen_) {
        #ifdef Q_OS_WIN
            int curDPI = screen_->logicalDotsPerInch();
            scale_ = (double)curDPI / (double)LOWEST_LDPI;
        #else
            // for Mac curScale always == 1
            scale_ = 1;
        #endif
    } else  {
        scale_ = 1.0;
    }


    listView_ = new QListView();
    listView_->setMouseTracking(true);
    listView_->setStyleSheet("background-color: rgba(255, 255, 255, 0);\nborder-top: none;\nborder-bottom: none;");
    listView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView_->setResizeMode(QListView::Adjust);
    locationsTrayMenuItemDelegate_ = new LocationsTrayMenuItemDelegate(this, scale_);
    locationsTrayMenuItemDelegate_->setFontForItems(font);
    listView_->setItemDelegate(locationsTrayMenuItemDelegate_);
    listView_->viewport()->installEventFilter(this);
    listView_->setModel(model);
    connect(listView_, &QListView::clicked, this, &LocationsTrayMenuWidget::onListViewClicked);

    upButton_ = new LocationsTrayMenuButton();
    downButton_ = new LocationsTrayMenuButton();
    downButton_->setType(1);
    connect(upButton_, &LocationsTrayMenuButton::clicked, this, &LocationsTrayMenuWidget::onScrollUpClick);
    connect(downButton_, &LocationsTrayMenuButton::clicked, this, &LocationsTrayMenuWidget::onScrollDownClick);

    QVBoxLayout *widgetLayout = new QVBoxLayout();
    widgetLayout->addWidget(upButton_);
    widgetLayout->addWidget(listView_);
    widgetLayout->addWidget(downButton_);
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(0);
    setLayout(widgetLayout);

    // These signals must be connected after listView_->setModel(modelWithoutBestLocation) since the order of slot calls matters here
    connect(model, &QAbstractItemModel::modelReset, this, &LocationsTrayMenuWidget::onModelChanged);
    connect(model, &QAbstractItemModel::rowsInserted, this, &LocationsTrayMenuWidget::onModelChanged);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &LocationsTrayMenuWidget::onModelChanged);
    connect(model, &QAbstractItemModel::layoutChanged, this, &LocationsTrayMenuWidget::onModelChanged);

    onModelChanged();
}

bool LocationsTrayMenuWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == listView_->viewport()) {
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
            listView_->clearSelection();
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

void LocationsTrayMenuWidget::onListViewClicked(const QModelIndex &index)
{
    if (index.isValid() && (index.flags() & Qt::ItemIsEnabled)) {
        emit locationSelected( qvariant_cast<LocationID>(index.data(gui_locations::kLocationId)));
    }
}

void LocationsTrayMenuWidget::onSubmenuActionTriggered(QAction *action)
{
    WS_ASSERT(currentSubmenu_ && action);
    if (!currentSubmenu_ || !action) {
        return;
    }
    emit locationSelected(qvariant_cast<LocationID>(action->data()));
}

void LocationsTrayMenuWidget::updateTableViewSelection()
{
    QPoint pt = listView_->viewport()->mapFromGlobal(QCursor::pos());
    QModelIndex ind = listView_->indexAt(pt);
    if (ind.isValid() && ind.column() == 0)
    {
        listView_->setCurrentIndex(ind);
    }
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onScrollUpClick()
{
    int ind = listView_->currentIndex().row();
    if (ind > 0)
    {
        listView_->setCurrentIndex(listView_->model()->index(ind - 1, 0));
    }
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onScrollDownClick()
{
    int ind = listView_->currentIndex().row();
    if (ind < (listView_->model()->rowCount() - 1))
    {
        listView_->setCurrentIndex(listView_->model()->index(ind + 1, 0));
    }
    updateButtonsState();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::onModelChanged()
{
    recalcSize();
    updateButtonsState();
    updateTableViewSelection();
    updateSubmenuForSelection();
    updateBackground_mac();
}

void LocationsTrayMenuWidget::recalcSize()
{
    QStyleOptionMenuItem opt;
    QSize sz;
    sz = QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz);
    const int scaledItemHeight = sz.height() * scale_;
    visibleItemsCount_ = listView_->model()->rowCount();
    if (visibleItemsCount_ < 1)
        visibleItemsCount_ = 1;

    if (screen_) {
        const int availableHeight = screen_->geometry().height() -
            upButton_->sizeHint().height() - downButton_->sizeHint().height();
        const int maxItemCount = availableHeight / scaledItemHeight - 1;
        if (visibleItemsCount_ > maxItemCount)
            visibleItemsCount_ = maxItemCount;
    }


    int maxWidth = 190 * scale_; // set initial mimimum size
    // if the size of any item exceeds maxWidth, then will increase maxWidth
    for (int i = 0; i < listView_->model()->rowCount(); ++i)
    {
        int width = locationsTrayMenuItemDelegate_->calcWidth(listView_->model()->index(i, 0));
        if (width > maxWidth)
        {
            maxWidth = width;
        }
    }

    listView_->setFixedSize(maxWidth, scaledItemHeight * visibleItemsCount_);
}

void LocationsTrayMenuWidget::updateSubmenuForSelection()
{
    QPersistentModelIndex curIndex = listView_->currentIndex();
    if (currentIndexForSubmenu_ != curIndex)
    {
        if (currentSubmenu_) {
            delete currentSubmenu_;
            currentSubmenu_ = nullptr;
            currentIndexForSubmenu_ = QPersistentModelIndex();
        }
        if (curIndex.isValid() && curIndex.data(gui_locations::kIsTopLevelLocation).toBool())
        {
            int citiesCount = curIndex.model()->rowCount(curIndex);
            if (citiesCount > 0)
            {
                auto *submenu = new LocationsTrayMenuWidgetSubmenu(this);
                for (int i = 0; i < citiesCount; ++i)
                {
                    QModelIndex cityInd = curIndex.model()->index(i, 0, curIndex);
                    QString cityName = cityInd.data().toString() + " - " + cityInd.data(gui_locations::kDisplayNickname).toString();
                    bool isShowAsPremium = cityInd.data(gui_locations::kIsShowAsPremium).toBool();
                    if (isShowAsPremium)
                    {
                        cityName += " (Pro)";
                    }
                    auto *action = submenu->addAction(cityName);
                    action->setEnabled(!isShowAsPremium && !cityInd.data(gui_locations::kIsDisabled).toBool());
                    action->setData(cityInd.data(gui_locations::kLocationId));
                }
                connect(submenu, &LocationsTrayMenuWidgetSubmenu::triggered,  this, &LocationsTrayMenuWidget::onSubmenuActionTriggered);
                currentSubmenu_ = submenu;
                const auto rc = listView_->visualRect(curIndex);
                QPoint popupPosition = listView_->viewport()->mapToGlobal(rc.topRight());
                currentSubmenu_->popup(popupPosition);
                currentIndexForSubmenu_ = curIndex;
            }
        }
    }
}

void LocationsTrayMenuWidget::updateButtonsState()
{
    QModelIndex ind = listView_->indexAt(QPoint(2,2));
    if (ind.isValid())
    {
        upButton_->setEnabled(ind.row() != 0);
        downButton_->setEnabled((ind.row() + visibleItemsCount_) < (listView_->model()->rowCount()));
    }

    const bool kAllowScrolling = listView_->model()->rowCount() > visibleItemsCount_;
    upButton_->setVisible(kAllowScrolling);
    downButton_->setVisible(kAllowScrolling);
}

void LocationsTrayMenuWidget::updateBackground_mac()
{
#ifdef Q_OS_MACOS
    // This is a hack to force repaint of the window background. For some reason, Qt doesn't do that
    // even when this is essential, e.g. before painting a widget with a transparent background,
    // such as |listWidget_|. It seems that such behaviour is OSX version dependent. This call
    // invalidates the whole submenu window to ensure the translucent menu background is repainted.
    WidgetUtils_mac::setNeedsDisplayForWindow(this);
#endif
}

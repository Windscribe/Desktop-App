#include "locationsview.h"
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"

#include <QPainter>

namespace gui_locations {

LocationsView::LocationsView(QWidget *parent) : QScrollArea(parent)
  //, model_(nullptr)
{
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE)); // scroll by this many px at a time
    //scrollBar_->setGeometry(WINDOW_WIDTH * G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), 170 * G_SCALE);
    //connect(scrollBar_, SIGNAL(handleDragged(int)), SLOT(onScrollBarHandleDragged(int)));
    //connect(scrollBar_, SIGNAL(stopScroll(bool)), SLOT(onScrollBarStopScroll(bool)));


    // widget
    widget_ = new ExpandableItemsWidget();
    setWidget(widget_);
    countryItemDelegate_ = new CountryItemDelegate();
    cityItemDelegate_ = new CityItemDelegate();
    widget_->setItemDelegate(countryItemDelegate_, cityItemDelegate_);
}

LocationsView::~LocationsView()
{
    delete countryItemDelegate_;
    delete cityItemDelegate_;
}

void LocationsView::setModel(QAbstractItemModel *model)
{
    widget_->setModel(model);
}

void LocationsView::setCountViewportItems(int cnt)
{
    //todo
}

void LocationsView::setShowLatencyInMs(bool isShowLatencyInMs)
{
    //todo
}

void LocationsView::setShowLocationLoad(bool isShowLocationLoad)
{
    //todo
}

void LocationsView::updateScaling()
{
    //todo
}

void LocationsView::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, QColor(255, 0 ,0));

}

void LocationsView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    widget_->setFixedWidth(size().width());
    scrollBar_->setFixedWidth(SCROLL_BAR_WIDTH * G_SCALE);
}

} // namespace gui_locations


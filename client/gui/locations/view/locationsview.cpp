#include "locationsview.h"
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"

#include <QPainter>

namespace gui {

LocationsView::LocationsView(QWidget *parent) : QScrollArea(parent)
  //, model_(nullptr)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    widget_ = new ExpandableItemsWidget();
    setWidget(widget_);

    countryItemDelegate_ = new CountryItemDelegate();
    widget_->setItemDelegateForExpandableItem(countryItemDelegate_);
    cityItemDelegate_ = new CityItemDelegate();
    widget_->setItemDelegateForNonExpandableItem(cityItemDelegate_);

    setMinimumWidth(WINDOW_WIDTH * G_SCALE + 150);
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



} // namespace gui


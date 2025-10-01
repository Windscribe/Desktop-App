#include "widgetswitcher.h"


namespace GuiLocations {

WidgetSwitcher::WidgetSwitcher(QWidget *parent, gui_locations::LocationsView *locationsView, EmptyListWidget *emptyListWidget) :
    QWidget(parent),
    locationsView_(locationsView),
    emptyListWidget_(emptyListWidget)
{
    locationsView->setParent(this);
    emptyListWidget->setParent(this);
    connect(locationsView, &gui_locations::LocationsView::emptyListStateChanged, this, &WidgetSwitcher::onEmptyListStateChanged);
    onEmptyListStateChanged(locationsView->isEmptyList());
}

void WidgetSwitcher::updateScaling()
{
    locationsView_->updateScaling();
    emptyListWidget_->updateScaling();
}

void WidgetSwitcher::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    locationsView_->setGeometry(0, 0, this->width(), this->height());
    emptyListWidget_->setGeometry(0, 0, this->width(), this->height());
}

void WidgetSwitcher::onEmptyListStateChanged(bool isEmptyList)
{
    if (isEmptyList) {
        emptyListWidget_->show();
        locationsView_->hide();
        emit widgetsSwicthed();
    } else {
        locationsView_->show();
        emptyListWidget_->hide();
        emit widgetsSwicthed();
    }
}

} // namespace GuiLocation

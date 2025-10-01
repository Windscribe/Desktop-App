#pragma once
#include <QWidget>
#include "locations/view/locationsview.h"
#include "emptylistwidget.h"

namespace GuiLocations {

// Toggles between two widgets. It is needed in order to switch between LocationsView and EmptyListWidget
// depending on whether there are items in LocationsView.
class WidgetSwitcher : public QWidget
{
    Q_OBJECT
public:
    // Takes ownership of locationsView and emptyListWidget
    explicit WidgetSwitcher(QWidget *parent, gui_locations::LocationsView *locationsView, EmptyListWidget *emptyListWidget);

    void updateScaling();
    gui_locations::LocationsView *locationsView() const { return locationsView_; }
    EmptyListWidget *emptyListWidget() const { return emptyListWidget_; }

signals:
    void widgetsSwicthed();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onEmptyListStateChanged(bool isEmptyList);

private:
    gui_locations::LocationsView *locationsView_;
    EmptyListWidget *emptyListWidget_;
};

} // namespace GuiLocation

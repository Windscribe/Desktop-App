#include "locationstraymenu.h"
#include "locationstraymenuwidget.h"

#include <QWidgetAction>

LocationsTrayMenu::LocationsTrayMenu(QWidget *parent, QAbstractItemModel *model, const QFont &font, const QRect &trayIconGeometry) :
    QMenu(parent)
{
    QWidgetAction *widgetAction = new QWidgetAction(this);
    LocationsTrayMenuWidget * locationsWidget = new LocationsTrayMenuWidget(nullptr, model, font, trayIconGeometry);
    connect(locationsWidget, &LocationsTrayMenuWidget::locationSelected, this, [this](const LocationID &lid) { emit locationSelected(lid); });
    widgetAction->setDefaultWidget(locationsWidget);
    addAction(widgetAction);
}

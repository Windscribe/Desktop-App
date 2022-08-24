#include "locationstraymenu.h"
#include "locationstraymenuwidget.h"

#include <QWidgetAction>

LocationsTrayMenu::LocationsTrayMenu(QAbstractItemModel *model, const QFont &font)
{
    QWidgetAction *widgetAction = new QWidgetAction(this);
    LocationsTrayMenuWidget * locationsWidget = new LocationsTrayMenuWidget(nullptr, model, font);
    connect(locationsWidget, &LocationsTrayMenuWidget::locationSelected, this, [this](const LocationID &lid) { emit locationSelected(lid); });
    widgetAction->setDefaultWidget(locationsWidget);
    addAction(widgetAction);
}

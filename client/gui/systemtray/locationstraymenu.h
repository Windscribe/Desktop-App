#pragma once

#include <QAbstractItemModel>
#include <QMenu>
#include "types/locationid.h"

class LocationsTrayMenu : public QMenu
{
    Q_OBJECT
public:
    explicit LocationsTrayMenu(QAbstractItemModel *model, const QFont &font, const QRect &trayIconGeometry);
signals:
    void locationSelected(const LocationID &lid);
};


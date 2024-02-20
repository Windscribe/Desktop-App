#pragma once

#include <QHash>
#include <QMenu>
#include <QVector>
#include <QAbstractItemModel>
#include "types/locationid.h"

class LocationsTrayMenuNative : public QMenu
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuNative(QWidget *parent, QAbstractItemModel *model);

signals:
    void locationSelected(const LocationID &lid);

private slots:
    void onMenuActionTriggered(bool checked);
    void onMenuTriggered(QAction *action);

private:
    void buildMenu(QAbstractItemModel *model);
};

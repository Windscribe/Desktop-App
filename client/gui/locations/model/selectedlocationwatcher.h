#pragma once

#include <QAbstractItemModel>
#include "types/locationid.h"
#include "types/pingtime.h"

namespace gui_locations {

// Monitors any changes in the selected item
class SelectedLocationWatcher : public QObject
{
    Q_OBJECT
public:
    explicit SelectedLocationWatcher(QObject *parent, QAbstractItemModel *model);
    bool setSelectedLocation(const LocationID &lid);

    bool isCorrect() const;

    LocationID locationdId() const { return id_; }
    QString firstName() const { return firstName_; }
    QString secondName() const { return secondName_; }
    QString countryCode() const { return countryCode_; }
    PingTime pingTime() const { return pingTime_; }

signals:
    void itemChanged();
    void itemRemoved();

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

private:
    QAbstractItemModel *model_;
    bool isIndexSettled_;
    QPersistentModelIndex selIndex_;

    LocationID id_;
    QString firstName_;
    QString secondName_;
    QString countryCode_;
    PingTime pingTime_;

    void fillData();
};

} //namespace gui_locations


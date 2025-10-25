#pragma once

#include <QAbstractItemModel>
#include "types/locationid.h"
#include "types/pingtime.h"

namespace gui_locations {

// Utils class, make a location selected and monitor its changes when there are corresponding changes in the model

class SelectedLocation : public QObject
{
    Q_OBJECT
public:
    explicit SelectedLocation(QAbstractItemModel *model);

    void set(const LocationID &lid);
    void clear();
    bool isValid() const { return isValid_; }

    LocationID locationdId() const { return id_; }
    LocationID prevLocationdId() const { return prevId_; }
    QString firstName() const { return firstName_; }
    QString secondName() const { return secondName_; }
    QString countryCode() const { return countryCode_; }
    QString shortName() const { return shortName_; }
    PingTime pingTime() const { return pingTime_; }

signals:
    void changed();
    void removed();

private slots:
    void checkForRemove();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

private:
    QAbstractItemModel *model_;
    bool isValid_;
    QPersistentModelIndex selIndex_;

    LocationID id_;
    LocationID prevId_;
    QString firstName_;
    QString secondName_;
    QString countryCode_;
    QString shortName_;
    PingTime pingTime_;

    void fillData();
    void setInvalid();
};

} //namespace gui_locations


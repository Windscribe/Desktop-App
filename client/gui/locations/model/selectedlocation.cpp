#include "selectedlocation.h"
#include "locations/locationsmodel_roles.h"
#include "locationsmodel.h"

namespace gui_locations {

SelectedLocation::SelectedLocation(QAbstractItemModel *model) : QObject(model),
    model_(model), isValid_(false)
{
    Q_ASSERT(model != nullptr);
    Q_ASSERT(dynamic_cast<LocationsModel *>(model) != nullptr);

    connect(model_, &QAbstractItemModel::modelReset, this, &SelectedLocation::checkForRemove);
    connect(model_, &QAbstractItemModel::rowsRemoved, this, &SelectedLocation::checkForRemove);
    connect(model_, &QAbstractItemModel::dataChanged, this, &SelectedLocation::onDataChanged);
}

void SelectedLocation::set(const LocationID &lid)
{
    selIndex_ = static_cast<LocationsModel *>(model_)->getIndexByLocationId(lid);
    if (selIndex_.isValid()) {
        isValid_ = true;
        fillData();
    }
    else {
        setInvalid();
    }
}

void SelectedLocation::checkForRemove()
{
    if (isValid_)
    {
        if (!selIndex_.isValid())
        {
            setInvalid();
            emit removed();
        }
    }
}

void SelectedLocation::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &/*roles*/)
{
    Q_ASSERT(topLeft.parent() == bottomRight.parent());
    Q_ASSERT(topLeft.column() == 0);
    Q_ASSERT(bottomRight.column() == 0);
    if (isValid_ && selIndex_.isValid())
    {
        if (selIndex_.parent() == topLeft.parent())
        {
            if (selIndex_.row() >= topLeft.row() && selIndex_.row() <= bottomRight.row())
            {
                fillData();
                emit changed();
            }
        }
    }
}

void SelectedLocation::fillData()
{
    Q_ASSERT(isValid_);
    Q_ASSERT(selIndex_.isValid());
    LocationID extractedLid = qvariant_cast<LocationID>(selIndex_.data(LOCATION_ID));
    prevId_ = id_;
    id_ = extractedLid;
    firstName_ = selIndex_.data(NAME).toString();
    secondName_ = selIndex_.data(NICKNAME).toString();
    countryCode_ = selIndex_.data(COUNTRY_CODE).toString();
    pingTime_ = selIndex_.data(PING_TIME).toInt();
}

void SelectedLocation::setInvalid()
{
    isValid_ = false;
    id_ = LocationID();
    prevId_ = LocationID();
    firstName_.clear();
    secondName_.clear();
    pingTime_ = PingTime::NO_PING_INFO;
}

} //namespace gui_locations

#include "selectedlocationwatcher.h"
#include "locations/locationsmodel_roles.h"
#include "locations/model/locationsmodel.h"
#include <QDebug>

namespace gui {

SelectedLocationWatcher::SelectedLocationWatcher(QObject *parent, QAbstractItemModel *model) : QObject(parent),
    model_(model), isIndexSettled_(false)
{
    connect(model_, &QAbstractItemModel::modelReset,
                [=]()
    {
        if (isIndexSettled_)
        {
            if (!selIndex_.isValid())
            {
                isIndexSettled_ = false;
                emit itemRemoved();
            }
        }
    });

    connect(model_, &QAbstractItemModel::rowsRemoved,
                [=]()
    {
        if (isIndexSettled_)
        {
            if (!selIndex_.isValid())
            {
                isIndexSettled_ = false;
                emit itemRemoved();
            }
        }
    });

    connect(model_, &QAbstractItemModel::dataChanged, this, &SelectedLocationWatcher::onDataChanged);
}

bool SelectedLocationWatcher::setSelectedLocation(const LocationID &lid)
{
    Q_ASSERT(dynamic_cast<gui::LocationsModel *>(model_) != nullptr);
    selIndex_ = static_cast<gui::LocationsModel *>(model_)->getIndexByLocationId(lid);
    if (selIndex_.isValid())
    {
        isIndexSettled_ = true;
        fillData();
        return true;
    }
    return false;
}

bool SelectedLocationWatcher::isCorrect() const
{
    return isIndexSettled_;
}

void SelectedLocationWatcher::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &/*roles*/)
{
    Q_ASSERT(topLeft.parent() == bottomRight.parent());
    Q_ASSERT(topLeft.column() == 0);
    Q_ASSERT(bottomRight.column() == 0);
    if (isIndexSettled_ && selIndex_.isValid())
    {
        if (selIndex_.parent() == topLeft.parent())
        {
            if (selIndex_.row() >= topLeft.row() && selIndex_.row() <= bottomRight.row())
            {
                fillData();
                emit itemChanged();
            }
        }
    }
}

void SelectedLocationWatcher::fillData()
{
    if (isIndexSettled_ && selIndex_.isValid())
    {
        LocationID extractedLid = qvariant_cast<LocationID>(selIndex_.data(gui::LOCATION_ID));
        id_ = extractedLid;
        firstName_ = selIndex_.data(gui::NAME).toString();
        secondName_ = selIndex_.data(gui::NICKNAME).toString();
        countryCode_ = selIndex_.data(gui::COUNTRY_CODE).toString();
        pingTime_ = selIndex_.data(gui::PING_TIME).toInt();
    }
}

} //namespace gui

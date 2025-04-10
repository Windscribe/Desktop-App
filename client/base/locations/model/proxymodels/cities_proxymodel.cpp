#include "cities_proxymodel.h"
#include "utils/ws_assert.h"

namespace gui_locations {

CitiesProxyModel::CitiesProxyModel(QObject *parent) : QAbstractProxyModel(parent),
    isBeginRemoveRowsCalled_(false), isBeginMoveRowsCalled_(false)
{
}

void CitiesProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (this->sourceModel() == sourceModel)
    {
        return;
    }

    beginResetModel();
    if (this->sourceModel())
    {
        for (const QMetaObject::Connection& discIter : std::as_const(sourceConnections_))
            disconnect(discIter);
    }
    sourceConnections_.clear();

    QAbstractProxyModel::setSourceModel(sourceModel);
    if (this->sourceModel())
    {
        using namespace std::placeholders;
        sourceConnections_ = QVector<QMetaObject::Connection>{
                connect(this->sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, &CitiesProxyModel::beginResetModel),
                connect(this->sourceModel(), &QAbstractItemModel::modelReset, this, &CitiesProxyModel::onModelReset),
                connect(this->sourceModel(), &QAbstractItemModel::dataChanged, this, &CitiesProxyModel::onDataChanged),
                connect(this->sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &CitiesProxyModel::onRowsAboutToBeRemoved),
                connect(this->sourceModel(), &QAbstractItemModel::rowsInserted, this, &CitiesProxyModel::onRowsInserted),
                connect(this->sourceModel(), &QAbstractItemModel::rowsRemoved, this, &CitiesProxyModel::onRowsRemoved),
                connect(this->sourceModel(), &QAbstractItemModel::rowsAboutToBeMoved, this, &CitiesProxyModel::onRowsAboutToBeMoved),
                connect(this->sourceModel(), &QAbstractItemModel::rowsMoved, this, &CitiesProxyModel::onRowsMoved)
            };
    }
    onModelReset();
}

QModelIndex CitiesProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();

    int ind = items_.indexOf(sourceIndex);
    WS_ASSERT(ind != -1);
    return createIndex(ind, 0);
}

QModelIndex CitiesProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();
    WS_ASSERT(proxyIndex.row() >= 0 && proxyIndex.row() < items_.count() && proxyIndex.column() == 0);
    return items_[proxyIndex.row()];
}

int CitiesProxyModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

QModelIndex CitiesProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    return createIndex(row, column);
 }

QModelIndex CitiesProxyModel::parent(const QModelIndex &/*index*/) const
{
    return QModelIndex();
}

int CitiesProxyModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return items_.count();
    else
        return 0;
}

bool CitiesProxyModel::hasChildren(const QModelIndex &parent) const
{
    return (!parent.isValid() && !items_.isEmpty());
}

void CitiesProxyModel::onModelReset()
{
    initModel();
    endResetModel();
}

void CitiesProxyModel::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    WS_ASSERT(topLeft.parent() == bottomRight.parent());
    if (topLeft.parent().isValid())
    {
        QModelIndex topLeftSelf = mapFromSource(topLeft);
        QModelIndex bottomRightSelf = mapFromSource(bottomRight);
        emit dataChanged(topLeftSelf, bottomRightSelf, roles);
    }
}

void CitiesProxyModel::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    WS_ASSERT(first == last);        // supported only one insertion

    if (!parent.isValid())
    {
        int internalInd = findInternalIndex(parent, first);
        QModelIndex sourceModelIndex = sourceModel()->index(first, 0, parent);
        int countChilds = sourceModel()->rowCount(sourceModelIndex);
        if (countChilds > 0)
        {
            beginInsertRows(QModelIndex(), internalInd, internalInd + countChilds - 1);
            for (int i = 0; i < countChilds; ++i)
            {
                items_.insert(internalInd + i, sourceModel()->index(i, 0, sourceModelIndex));
            }
            endInsertRows();
        }
    }
    else
    {
        int internalInd = findInternalIndex(parent, first);
        beginInsertRows(QModelIndex(), internalInd, internalInd);
        items_.insert(internalInd, sourceModel()->index(first, 0, parent));
        endInsertRows();
    }
}

void CitiesProxyModel::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    WS_ASSERT(first == last);        // supported only one removal

    if (!parent.isValid())
    {
        int internalInd = findInternalIndex(parent, first);
        QModelIndex sourceModelIndex = sourceModel()->index(first, 0, parent);
        int countChilds = sourceModel()->rowCount(sourceModelIndex);
        if (countChilds > 0)
        {
            beginRemoveRows(QModelIndex(), internalInd, internalInd + countChilds - 1);
            items_.remove(internalInd, countChilds);
            isBeginRemoveRowsCalled_ = true;
        }
    }
    else
    {
        int internalInd = findInternalIndex(parent, first);
        beginRemoveRows(QModelIndex(), internalInd, internalInd);
        items_.remove(internalInd);
        isBeginRemoveRowsCalled_ = true;
    }
}

void CitiesProxyModel::onRowsRemoved(const QModelIndex &/*parent*/, int /*first*/, int /*last*/)
{
    if (isBeginRemoveRowsCalled_)
    {
        isBeginRemoveRowsCalled_ = false;
        endRemoveRows();
    }
}

void CitiesProxyModel::onRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    WS_ASSERT(sourceParent == destinationParent);
    WS_ASSERT(sourceStart == sourceEnd);        // supported only one item move

    if (!sourceParent.isValid())
    {
        int internalInd = findInternalIndex(sourceParent, sourceStart);
        QModelIndex sourceModelIndex = sourceModel()->index(sourceStart, 0, sourceParent);
        int countChilds = sourceModel()->rowCount(sourceModelIndex);
        if (countChilds > 0)
        {
            int destInd = findInternalIndex(sourceParent, destinationRow);
            beginMoveRows(QModelIndex(), internalInd, internalInd + countChilds - 1, QModelIndex(), destInd);

            for (int i = internalInd; i < (internalInd + countChilds); i++)
            {
                items_.move(i, destInd + i - internalInd);
            }
            isBeginMoveRowsCalled_ = true;
        }
    }
    else
    {
        int internalInd = findInternalIndex(sourceParent, sourceStart);
        int destInd = findInternalIndex(sourceParent, destinationRow);
        beginMoveRows(QModelIndex(), internalInd, internalInd, QModelIndex(), destInd);
        items_.move(internalInd, destInd);
        isBeginMoveRowsCalled_ = true;
    }
}

void CitiesProxyModel::onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row)
{
    if (isBeginMoveRowsCalled_)
    {
        isBeginMoveRowsCalled_ = false;
        endMoveRows();
    }
}

void CitiesProxyModel::initModel()
{
    items_.clear();

    if (sourceModel())
    {
        for (int i = 0, rowCnt = sourceModel()->rowCount(); i < rowCnt; ++i)
        {
            QModelIndex mi = sourceModel()->index(i, 0);
            for (int k = 0; k < sourceModel()->rowCount(mi); ++k)
            {
                items_ << sourceModel()->index(k, 0, mi);
            }
        }
    }
}

int CitiesProxyModel::findInternalIndex(const QModelIndex &parent, int row)
{
    int cnt = 0;
    for (int i = 0, rowCnt = sourceModel()->rowCount(); i < rowCnt; ++i)
    {
        if (!parent.isValid() && row == i)
        {
            return cnt;
        }
        QModelIndex mi = sourceModel()->index(i, 0);
        if (parent == mi)
        {
            cnt += row;
            return cnt;
        }
        cnt += sourceModel()->rowCount(mi);
    }
    WS_ASSERT(false);
    return -1;
}

} //namespace gui_locations


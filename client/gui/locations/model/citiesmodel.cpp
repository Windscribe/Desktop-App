#include "citiesmodel.h"

namespace gui {

CitiesModel::CitiesModel(QObject *parent) : QAbstractProxyModel(parent),
    isBeginRemoveRowsCalled_(false)
{
}

void CitiesModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (this->sourceModel() == sourceModel)
    {
        return;
    }

    beginResetModel();
    if (this->sourceModel())
    {
        for (const QMetaObject::Connection& discIter : qAsConst(sourceConnections_))
            disconnect(discIter);
    }
    sourceConnections_.clear();

    QAbstractProxyModel::setSourceModel(sourceModel);
    if (this->sourceModel())
    {
        using namespace std::placeholders;
        sourceConnections_ = QVector<QMetaObject::Connection>{
                connect(this->sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, &CitiesModel::beginResetModel),
                connect(this->sourceModel(), &QAbstractItemModel::modelReset, this, &CitiesModel::onModelReset),
                connect(this->sourceModel(), &QAbstractItemModel::dataChanged, this, &CitiesModel::onDataChanged),
                connect(this->sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &CitiesModel::onRowsAboutToBeRemoved),
                connect(this->sourceModel(), &QAbstractItemModel::rowsInserted, this, &CitiesModel::onRowsInserted),
                connect(this->sourceModel(), &QAbstractItemModel::rowsRemoved, this, &CitiesModel::onRowsRemoved)
            };
    }
    onModelReset();
}

QModelIndex CitiesModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
           return QModelIndex();

    Q_ASSERT(mapFromSourceModel_.contains(sourceIndex));
    int ind = mapFromSourceModel_[sourceIndex];
    return createIndex(ind, 0);
}

QModelIndex CitiesModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
           return QModelIndex();
    Q_ASSERT(proxyIndex.row() >= 0 && proxyIndex.row() < mapToSourceModel_.count() && proxyIndex.column() == 0);
    return mapToSourceModel_[proxyIndex.row()];
}

int CitiesModel::columnCount(const QModelIndex &/*parent*/) const
{
    if (mapToSourceModel_.isEmpty())
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

QModelIndex CitiesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    return createIndex(row, column);
 }

QModelIndex CitiesModel::parent(const QModelIndex &/*index*/) const
{
    return QModelIndex();
}

int CitiesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return mapToSourceModel_.count();
    else
        return 0;
}

bool CitiesModel::hasChildren(const QModelIndex &parent) const
{
    return (!parent.isValid() && !mapToSourceModel_.isEmpty());
}

void CitiesModel::onModelReset()
{
    initModel();
    endResetModel();
}

void CitiesModel::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_ASSERT(topLeft.parent() == bottomRight.parent());
    if (topLeft.parent().isValid())
    {
        QModelIndex topLeftSelf = mapFromSource(topLeft);
        QModelIndex bottomRightSelf = mapFromSource(bottomRight);
        emit dataChanged(topLeftSelf, bottomRightSelf, roles);
    }
}

void CitiesModel::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    int cnt = 0;
    int insert_ind = 0;

    if (!parent.isValid())
    {
        for (int i = first; i <= last; i++)
        {
            QModelIndex mi = sourceModel()->index(i, 0, parent);
            cnt += sourceModel()->rowCount(mi);
        }

        if (cnt > 0)
        {
            QModelIndex prevInd = findPrev(sourceModel(), parent, first);
            if (prevInd.isValid())
            {
                insert_ind = mapFromSource(prevInd).row() + 1;
            }
        }
    }
    else
    {
        cnt = last - first + 1;

        if (cnt > 0)
        {
            QModelIndex prevInd = findPrev(sourceModel(), parent, first);
            if (prevInd.isValid())
            {
                insert_ind = mapFromSource(prevInd).row() + 1;
            }
        }
    }


    if (cnt > 0)
    {
        beginInsertRows(QModelIndex(), insert_ind, insert_ind + cnt - 1);
        initModel();
        endInsertRows();
    }
}

void CitiesModel::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    if (!parent.isValid())
    {
        int rowCount = 0;
        for (int i = first; i <= last; ++i)
        {
            QModelIndex sourceMi = sourceModel()->index(i, 0, parent);
            rowCount += sourceModel()->rowCount(sourceMi);
        }

        if (rowCount > 0)
        {
            QModelIndex sourceMi = sourceModel()->index(first, 0, parent);
            QModelIndex mi = mapFromSource(sourceModel()->index(0, 0, sourceMi));
            isBeginRemoveRowsCalled_ = true;
            beginRemoveRows(QModelIndex(), mi.row(), mi.row() + rowCount - 1);
        }
    }
    else
    {
        QModelIndex sourceFirst = sourceModel()->index(first, 0, parent);
        QModelIndex sourceLast = sourceModel()->index(last, 0, parent);

        QModelIndex miFirst = mapFromSource(sourceFirst);
        QModelIndex miLast = mapFromSource(sourceLast);

        isBeginRemoveRowsCalled_ = true;
        beginRemoveRows(QModelIndex(), miFirst.row(), miLast.row());
    }
}

void CitiesModel::onRowsRemoved(const QModelIndex &/*parent*/, int /*first*/, int /*last*/)
{
    if (isBeginRemoveRowsCalled_)
    {
        isBeginRemoveRowsCalled_ = false;
        initModel();
        endRemoveRows();
    }
}

void CitiesModel::initModel()
{
    mapToSourceModel_.clear();
    mapFromSourceModel_.clear();

    if (sourceModel())
    {
        int cnt = 0;
        for (int i = 0; i < sourceModel()->rowCount(); ++i)
        {
            QModelIndex mi = sourceModel()->index(i, 0);

            for (int k = 0; k < sourceModel()->rowCount(mi); ++k)
            {
                mapToSourceModel_[cnt] = sourceModel()->index(k, 0, mi);
                mapFromSourceModel_[sourceModel()->index(k, 0, mi)] = cnt;
                cnt++;
            }
        }
    }
}

QModelIndex CitiesModel::findPrev(QAbstractItemModel *model, const QModelIndex &parent, int row)
{
    QModelIndex prevInd;

    if (row > 0)
    {
        prevInd = model->index(row - 1, 0, parent);
    }
    else if (row == 0)
    {
        if (parent.row() > 0)
        {
            prevInd = model->index(parent.row() - 1, 0);
        }
    }
    else
    {
        Q_ASSERT(false);
    }

    if (prevInd.isValid())
    {
        if (prevInd.parent().isValid())
        {
            return prevInd;
        }
        else
        {
            int rowCount = model->rowCount(prevInd);
            if (rowCount > 0)
            {
                return model->index(rowCount - 1, 0, prevInd);
            }
            else
            {
                return findPrev(model, QModelIndex(), prevInd.row());
            }
        }
    }
    else
    {
        return prevInd;
    }
}

} //namespace gui


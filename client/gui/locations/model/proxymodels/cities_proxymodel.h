#pragma once

#include <QAbstractProxyModel>

namespace gui_locations {

// Transforms LocationsModel to a linear list of cities (excluding countries)
class CitiesProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit CitiesProxyModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

private slots:
    void onModelReset();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);


private:
    QVector<QPersistentModelIndex> items_;
    bool isBeginRemoveRowsCalled_;
    bool isBeginMoveRowsCalled_;
    QVector<QMetaObject::Connection> sourceConnections_;

    void initModel();
    int findInternalIndex(const QModelIndex &parent, int row);
};

} //namespace gui_locations


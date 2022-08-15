#ifndef GUI_LOCATIONS_CITIESMODEL_H
#define GUI_LOCATIONS_CITIESMODEL_H

#include <QAbstractProxyModel>

namespace gui_location {

// Transforms LocationsModel to a linear list of cities (excluding countries)
class CitiesModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit CitiesModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QModelIndex	mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex	mapToSource(const QModelIndex &proxyIndex) const override;

    int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	parent(const QModelIndex &index) const override;
    int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

private slots:
    void onModelReset();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);

private:
    QMap<int, QModelIndex> mapToSourceModel_;
    QMap<QModelIndex, int> mapFromSourceModel_;
    bool isBeginRemoveRowsCalled_;
    QVector<QMetaObject::Connection> sourceConnections_;

    void initModel();

    // Find the previous index of item that a child
    QModelIndex findPrev(QAbstractItemModel *model, const QModelIndex &parent, int row);
};

} //namespace gui_location

#endif // GUI_LOCATIONS_CITIESMODEL_H

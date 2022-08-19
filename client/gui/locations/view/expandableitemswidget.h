#pragma once

#include <QWidget>
#include <QAbstractItemModel>
#include <QSet>
#include "iitemdelegate.h"
#include "cursorupdatehelper.h"

namespace gui_locations {

// widget where items of QAbstractItemModel are drawn, supports animated expansion
// supports tree with one level of children or list
// the size of the widget changes through the setMinimumSize() function
class ExpandableItemsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExpandableItemsWidget(QWidget *parent = nullptr);
    ~ExpandableItemsWidget() override;

    // does not take ownership of model or delegates
    void setModel(QAbstractItemModel *model);
    void setItemDelegateForExpandableItem(IItemDelegate *itemDelegate);
    void setItemDelegateForNonExpandableItem(IItemDelegate *itemDelegate);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QScopedPointer<CursorUpdateHelper> cursorUpdateHelper_;
    QVector<QPersistentModelIndex> items_;
    QSet<QPersistentModelIndex> expandedItems_;

    QAbstractItemModel *model_;
    IItemDelegate *expandableItemDelegate_;
    IItemDelegate *nonexpandableItemDelegate_;

    QPersistentModelIndex selectedInd_;

    int curWidth_;
    int curHeight_;

    bool bMousePressed_;
    QPersistentModelIndex mousePressedInd_;
    int mousePressedClickableId_;


    void updateItemsList();
    void sortItemsListByRow();
    QPersistentModelIndex detectSelectedItem(const QPoint &pt, QRect *outputRect = nullptr);
    IItemDelegate *delegateForItem(const QPersistentModelIndex &ind);
    bool isExpandableItem(const QPersistentModelIndex &ind);
    void updateWidgetSize();

};

} // namespace gui_locations


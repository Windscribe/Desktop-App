#pragma once

#include <QWidget>
#include <QAbstractItemModel>
#include <QSet>
#include <QVariantAnimation>
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
    void setItemDelegate(IItemDelegate *itemDelegateExpandableItem, IItemDelegate *itemDelegateNonExpandableItem);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onExpandingAnimationValueChanged(const QVariant &value);
    void onExpandingAnimationFinished();

private:
    QScopedPointer<CursorUpdateHelper> cursorUpdateHelper_;

    QVector<QPersistentModelIndex> items_;
    QSet<QPersistentModelIndex> expandedItems_;
    QPersistentModelIndex selectedInd_;

    QPersistentModelIndex expandingItem_;
    int expandingCurrentHeight_;
    QVariantAnimation expandingAnimation_;

    QAbstractItemModel *model_;
    IItemDelegate *expandableItemDelegate_;
    IItemDelegate *nonexpandableItemDelegate_;

    bool bMousePressed_;
    QPersistentModelIndex mousePressedItem_;
    int mousePressedClickableId_;

    void updateItemsList();
    void sortItemsListByRow();
    QPersistentModelIndex detectSelectedItem(const QPoint &pt, QRect *outputRect = nullptr);
    IItemDelegate *delegateForItem(const QPersistentModelIndex &ind);
    bool isExpandableItem(const QPersistentModelIndex &ind);
    void updateHeight();
    int calcHeightOfChildItems(const QPersistentModelIndex &ind);

};

} // namespace gui_locations


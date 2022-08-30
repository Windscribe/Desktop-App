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
    explicit ExpandableItemsWidget(QWidget *parent, QAbstractItemModel *model);
    ~ExpandableItemsWidget() override;

    // does not take ownership of delegates
    void setItemDelegate(IItemDelegate *itemDelegateExpandableItem, IItemDelegate *itemDelegateNonExpandableItem);
    void setItemHeight(int height);

    void setShowLatencyInMs(bool isShowLatencyInMs);
    void setShowLocationLoad(bool isShowLocationLoad);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void ensureVisible(int top, int bottom);
    void expandingAnimationStarted();

private slots:
    void onExpandingAnimationValueChanged(const QVariant &value);
    void onExpandingAnimationFinished();

private:
    static constexpr int kExpandingAnimationDuration = 200*10;
    QScopedPointer<CursorUpdateHelper> cursorUpdateHelper_;

    int itemHeight_;
    bool isShowLatencyInMs_;
    bool isShowLocationLoad_;

    QVector<QPersistentModelIndex> items_;
    QHash<QPersistentModelIndex, QSharedPointer<IItemCacheData>> itemsCacheData_;
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

    bool isAnimationJustStarted_ = false;


    void setModel(QAbstractItemModel *model);
    void resetItemsList();
    void sortItemsListByRow();
    QPersistentModelIndex detectSelectedItem(const QPoint &pt, QRect *outputRect = nullptr);
    IItemDelegate *delegateForItem(const QPersistentModelIndex &ind);
    bool isExpandableItem(const QPersistentModelIndex &ind);
    void updateHeight();
    int calcHeightOfChildItems(const QPersistentModelIndex &ind);
    int getOffsForItem(const QPersistentModelIndex &ind);
    void initCacheDataForChilds(const QPersistentModelIndex &parentInd);
    void clearCacheDataForChilds(const QPersistentModelIndex &parentInd);

    void updateExpandingAnimationParams();
    void setupExpandingAnimation(QAbstractAnimation::Direction direction, int startValue, int endValue, int duration);

};

} // namespace gui_locations


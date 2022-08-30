#include "expandableitemswidget.h"
#include "clickableandtooltiprects.h"
#include "../locationsmodel_roles.h"

#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QRegion>
#include <QElapsedTimer>

//FIXME:
long g_cntFrames = 0;
qint64 g_time = 0;

namespace gui_locations {

ExpandableItemsWidget::ExpandableItemsWidget(QWidget *parent, QAbstractItemModel *model) : QWidget(parent)
  , cursorUpdateHelper_(new CursorUpdateHelper(this))
  , itemHeight_(0)
  , isShowLatencyInMs_(false)
  , isShowLocationLoad_(false)
  , model_(nullptr)
  , expandableItemDelegate_(nullptr)
  , nonexpandableItemDelegate_(nullptr)
  , bMousePressed_(false)
  , mousePressedClickableId_(-1)
{
    setMouseTracking(true);
    setModel(model);

    connect(&expandingAnimation_, &QVariantAnimation::valueChanged, this, &ExpandableItemsWidget::onExpandingAnimationValueChanged);
    connect(&expandingAnimation_, &QVariantAnimation::finished, this, &ExpandableItemsWidget::onExpandingAnimationFinished);
}

ExpandableItemsWidget::~ExpandableItemsWidget()
{
    qDebug() << (double)g_time / (double)g_cntFrames;
}

void ExpandableItemsWidget::setModel(QAbstractItemModel *model)
{
    Q_ASSERT(model_ == nullptr);
    model_ = model;

    connect(model_, &QAbstractItemModel::modelReset, [this]()  {
        resetItemsList();
    });

    connect(model_, &QAbstractItemModel::rowsInserted, [this](const QModelIndex &parent, int first, int last) {
        Q_ASSERT(first == last);   // not supported more than one item
        if (!parent.isValid()) {
            QModelIndex mi = model_->index(first, 0);
            items_.insert(first, mi);
            itemsCacheData_[mi] = QSharedPointer<IItemCacheData>(delegateForItem(mi)->createCacheData(mi));
            initCacheDataForChilds(mi);
        }
        else {
            Q_ASSERT(items_.indexOf(parent) != -1);
            initCacheDataForChilds(parent);
        }
        updateExpandingAnimationParams();
        updateHeight();
        update();
    });

    connect(model_, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex &parent, int first, int last) {
        Q_ASSERT(first == last);   // not supported more than one item
        if (!parent.isValid())
            clearCacheDataForChilds(model_->index(first, 0));
        else {
            QPersistentModelIndex childInd = model_->index(first, 0, parent);
            itemsCacheData_.remove(childInd);
        }
    });
    connect(model_, &QAbstractItemModel::rowsRemoved, [this](const QModelIndex &parent, int first, int last) {
        Q_ASSERT(first == last);   // not supported more than one item
        if (!parent.isValid()) {
            itemsCacheData_.remove(items_[first]);
            items_.remove(first);
        }
        updateExpandingAnimationParams();
        updateHeight();
        update();
    });

    connect(model_, &QAbstractItemModel::rowsMoved, [this]() {
        Q_ASSERT(false);  // not supported
    });

    connect(model_, &QAbstractItemModel::dataChanged, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
        if (!topLeft.isValid() || !bottomRight.isValid()) {
            return;
        }
        Q_ASSERT(topLeft.parent() == bottomRight.parent());
        // update cache data
        for (int i = topLeft.row(); i <= bottomRight.row(); ++i)
        {
            QPersistentModelIndex mi = model_->index(i, 0, topLeft.parent());
            delegateForItem(mi)->updateCacheData(mi, itemsCacheData_[mi].get());
        }
        update();
    });

    connect(model_, &QAbstractItemModel::layoutChanged, [this]() {
        // sort items by row
        std::sort(items_.begin(), items_.end(), [](const QPersistentModelIndex & a, const QPersistentModelIndex & b) -> bool {
            return a.row() < b.row();
        });
        update();
    });

    resetItemsList();
}

void ExpandableItemsWidget::setItemDelegate(IItemDelegate *itemDelegateExpandableItem, IItemDelegate *itemDelegateNonExpandableItem)
{
    expandableItemDelegate_ = itemDelegateExpandableItem;
    nonexpandableItemDelegate_ = itemDelegateNonExpandableItem;
}

void ExpandableItemsWidget::setItemHeight(int height)
{
    itemHeight_ = height;
    updateHeight();
    update();
}

void ExpandableItemsWidget::setShowLatencyInMs(bool isShowLatencyInMs)
{
    isShowLatencyInMs_ = isShowLatencyInMs;
    update();
}

void ExpandableItemsWidget::setShowLocationLoad(bool isShowLocationLoad)
{
    isShowLocationLoad_ = isShowLocationLoad;
    update();
}

void ExpandableItemsWidget::paintEvent(QPaintEvent *event)
{
    QElapsedTimer elapsed;
    elapsed.start();

    QPainter painter(this);

    int topOffs = 0;
    for (const auto &it : qAsConst(items_))
    {
        IItemDelegate *delegate = delegateForItem(it);
        QRect rcItem(0, topOffs, size().width(), itemHeight_);

        bool isExpanded = expandedItems_.contains(it);
        double expandedProgress;
        if (it == expandingItem_) {
            expandedProgress = (double)expandingCurrentHeight_ / (double)expandingAnimation_.endValue().toInt();
        }
        else {
            expandedProgress = isExpanded ? 1.0 : 0.0;
        }

        //if (rcItem.intersects(event->rect()))
        {
            ItemStyleOption opt(rcItem, it == selectedInd_ ? 1.0 : 0.0, expandedProgress, isShowLocationLoad_, isShowLatencyInMs_);
            delegate->paint(&painter, opt, it, itemsCacheData_[it].get());
        }
        topOffs += itemHeight_;

        // draw child items if this item is expanded
        if (isExpanded)
        {
            if (it == expandingItem_)
            {
                painter.setClipRect(QRect(0, topOffs, size().width(), expandingCurrentHeight_));
            }
            int row = 0;
            int overallHeight = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid())
            {
                QRect rcChildItem(0, topOffs + overallHeight, size().width(), itemHeight_);
                //if (rcChildItem.intersects(event->rect()))
                {
                    ItemStyleOption opt(rcChildItem, childInd == selectedInd_ ? 1.0 : 0.0, false, isShowLocationLoad_, isShowLatencyInMs_);
                    nonexpandableItemDelegate_->paint(&painter, opt, childInd, itemsCacheData_[childInd].get());
                }
                overallHeight += itemHeight_;
                row++;
                childInd = it.model()->index(row, 0, it);
            }

            if (it == expandingItem_)
            {
                painter.setClipping(false);
                topOffs += expandingCurrentHeight_;
            }
            else
            {
                topOffs += overallHeight;
            }
        }
    }
    g_time += elapsed.elapsed();
    g_cntFrames++;
}

void ExpandableItemsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (bMousePressed_) {
        return;
    }

    QPersistentModelIndex sel = detectSelectedItem(event->pos());
    if (sel != selectedInd_)
    {
        selectedInd_ = sel;

        if (selectedInd_.isValid() && delegateForItem(selectedInd_)->isForbiddenCursor(selectedInd_))
        {
            cursorUpdateHelper_->setForbiddenCursor();
        }
        else
        {
            cursorUpdateHelper_->setPointingHandCursor();
        }

        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ExpandableItemsWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QRect rcItem;
        mousePressedItem_ = detectSelectedItem(event->pos(), &rcItem);
        if (mousePressedItem_.isValid())
        {
            mousePressedClickableId_ = delegateForItem(mousePressedItem_)->isInClickableArea(mousePressedItem_, event->pos() - rcItem.topLeft(), rcItem);
            bMousePressed_ = true;
        }
    }

    QWidget::mousePressEvent(event);
}

void ExpandableItemsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && bMousePressed_)
    {
        QRect rcItem;
        if (mousePressedItem_ == detectSelectedItem(event->pos(), &rcItem))
        {
            if (isExpandableItem(mousePressedItem_))
            {
                if (expandingAnimation_.state() != QAbstractAnimation::Running)
                {
                    emit expandingAnimationStarted();

                    if (expandedItems_.contains(mousePressedItem_))
                    {
                        expandingItem_ = mousePressedItem_;
                        expandingCurrentHeight_ = calcHeightOfChildItems(expandingItem_);
                        setupExpandingAnimation(QAbstractAnimation::Backward, 0, expandingCurrentHeight_, kExpandingAnimationDuration);
                        expandingAnimation_.start();
                    }
                    else
                    {
                        isAnimationJustStarted_ = true;
                        expandedItems_.insert(mousePressedItem_);
                        expandingItem_ = mousePressedItem_;
                        expandingCurrentHeight_ = 0;
                        int heightOfChilds = calcHeightOfChildItems(expandingItem_);
                        setupExpandingAnimation(QAbstractAnimation::Forward, 0, heightOfChilds, kExpandingAnimationDuration);
                        expandingAnimation_.start();
                    }

                    updateHeight();
                    update();
                }
            }
            else    // non expandable item
            {
                // todo city click
                int clickableId = delegateForItem(mousePressedItem_)->isInClickableArea(mousePressedItem_, event->pos() - rcItem.topLeft(), rcItem);
                if (clickableId != -1 && clickableId == mousePressedClickableId_)
                {
                    if (clickableId == CLICKABLE_FAVORITE_RECT)
                    {
                        model_->setData(mousePressedItem_, !mousePressedItem_.data(kIsFavorite).toBool(), kIsFavorite);
                    }
                }
            }
        }
        bMousePressed_ = false;
        mouseMoveEvent(event);  // detect selected item
    }

    QWidget::mouseReleaseEvent(event);
}

void ExpandableItemsWidget::onExpandingAnimationValueChanged(const QVariant &value)
{
    expandingCurrentHeight_ = value.toInt();
    updateHeight();
    update();

    // the expanding items should be visible so sending the signal to LocationsView
    // to update the scroll position
    if (expandingAnimation_.direction() == QAbstractAnimation::Forward) {

        int offs = getOffsForItem(expandingItem_);

        if (isAnimationJustStarted_) {
            isAnimationJustStarted_ = false;
            if (expandingCurrentHeight_ != 0)
            {
                int g = 0;
            }
        }


        emit ensureVisible(offs, offs + expandingCurrentHeight_ + itemHeight_);
    }

}

void ExpandableItemsWidget::onExpandingAnimationFinished()
{
    if (expandingAnimation_.direction() == QAbstractAnimation::Backward)
    {
        expandedItems_.remove(expandingItem_);
    }
    expandingItem_ = QModelIndex();
    updateHeight();
    update();
}

void ExpandableItemsWidget::resetItemsList()
{
    if (expandingAnimation_.state() == QAbstractAnimation::Running)
    {
        expandingAnimation_.stop();
        expandingItem_ = QModelIndex();
        selectedInd_ = QModelIndex();
    }

    items_.clear();
    itemsCacheData_.clear();
    expandedItems_.clear();
    for (int i = 0, rows_cnt = model_->rowCount(); i < rows_cnt; ++i)
    {
        QModelIndex mi = model_->index(i, 0);
        items_ << mi;
        itemsCacheData_[mi] = QSharedPointer<IItemCacheData>(delegateForItem(mi)->createCacheData(mi));
        initCacheDataForChilds(mi);
    }

    updateHeight();
    update();
}

void ExpandableItemsWidget::sortItemsListByRow()
{
    std::sort(items_.begin(), items_.end(),
        [](const QPersistentModelIndex & a, const QPersistentModelIndex & b) -> bool
    {
        return a.row() < b.row();
    });
}

QPersistentModelIndex ExpandableItemsWidget::detectSelectedItem(const QPoint &pt, QRect *outputRect)
{
    int top = 0;
    for (auto it : qAsConst(items_))
    {
        QRect rcItem = QRect(0, top, size().width(), itemHeight_);
        if (rcItem.contains(pt))
        {
            if (outputRect) {
                *outputRect = rcItem;
            }
            return it;
        }
        top += itemHeight_;

        if (expandedItems_.contains(it))
        {
            int row = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid())
            {
                QRect rcChildItem(0, top, size().width(), itemHeight_);
                if (rcChildItem.contains(pt))
                {
                    if (outputRect) {
                        *outputRect = rcChildItem;
                    }
                    return childInd;
                }
                top += itemHeight_;
                row++;
                childInd = it.model()->index(row, 0, it);
            }
        }

    }
    return QPersistentModelIndex();
}

IItemDelegate *ExpandableItemsWidget::delegateForItem(const QPersistentModelIndex &ind)
{
    // Is it an expandable item?
    if (isExpandableItem(ind)) {
        return expandableItemDelegate_;
    }
    else {
        return nonexpandableItemDelegate_;
    }
}

bool ExpandableItemsWidget::isExpandableItem(const QPersistentModelIndex &ind)
{
    return ind.data(gui_locations::kIsTopLevelLocation).toBool();
}

void ExpandableItemsWidget::updateHeight()
{
    int curHeight = 0;
    for (const auto &it : qAsConst(items_)) {
        curHeight += itemHeight_;
        if (expandedItems_.contains(it)) {
            if (it == expandingItem_)
                curHeight += expandingCurrentHeight_;
            else
                curHeight += calcHeightOfChildItems(it);
        }
    }
    resize(size().width(), curHeight);
}

int ExpandableItemsWidget::calcHeightOfChildItems(const QPersistentModelIndex &ind)
{
    return model_->rowCount(ind) * itemHeight_;
}

int ExpandableItemsWidget::getOffsForItem(const QPersistentModelIndex &ind)
{
    int top = 0;
    for (const auto &it : qAsConst(items_)) {
        if (it == ind)
            return top;
        top += itemHeight_;
        if (expandedItems_.contains(it))
            top += model_->rowCount(it) * itemHeight_;
    }
    Q_ASSERT(false);
    return 0;
}

void ExpandableItemsWidget::initCacheDataForChilds(const QPersistentModelIndex &parentInd)
{
    for (int c = 0, childs_cnt = model_->rowCount(parentInd); c < childs_cnt; ++c )
    {
        QModelIndex childMi = model_->index(c, 0, parentInd);
        itemsCacheData_[childMi] = QSharedPointer<IItemCacheData>(delegateForItem(childMi)->createCacheData(childMi));
    }
}

void ExpandableItemsWidget::clearCacheDataForChilds(const QPersistentModelIndex &parentInd)
{
    for (int c = 0, childs_cnt = model_->rowCount(parentInd); c < childs_cnt; ++c )
    {
        QModelIndex childMi = model_->index(c, 0, parentInd);
        auto it = itemsCacheData_.constFind(childMi);
        Q_ASSERT(it != itemsCacheData_.end());
        itemsCacheData_.erase(it);
    }
}

void ExpandableItemsWidget::updateExpandingAnimationParams()
{
    //FIXME:
    /*if (expandingAnimation_.state() != QAbstractAnimation::Running)
        return;

    // check if expanding item is deleted
    if (!expandingItem_.isValid()) {
        expandingAnimation_.stop();
        expandingItem_ = QModelIndex();
        return;
    }


    int heightOfChilds = calcHeightOfChildItems(expandingItem_);
    int endValue = expandingAnimation_.endValue().toInt();

    if (expandingAnimation_.direction() == QAbstractAnimation::Forward) {
        if (heightOfChilds != endValue) {
            double progress = (double)expandingAnimation_.currentTime() / (double)expandingAnimation_.totalDuration();
            int curValue = expandingAnimation_.currentValue().toInt();

            expandingAnimation_.stop();
            if (curValue > heightOfChilds)
                expandingItem_ = QModelIndex();
            else
            {
                expandingAnimation_.setStartValue(curValue);
                expandingCurrentHeight_ = curValue;
                expandingAnimation_.setEndValue(heightOfChilds);
                expandingAnimation_.setDuration(kExpandingAnimationDuration * (1.0 - progress));
                expandingAnimation_.start();
                int offs = getOffsForItem(expandingItem_);
            }
        }
    }
    else { // QAbstractAnimation::Backward
        if (heightOfChilds != endValue) {
            double progress = (double)expandingAnimation_.currentTime() / (double)expandingAnimation_.totalDuration();
            int newCurValue = expandingAnimation_.currentValue().toInt() < heightOfChilds ? expandingAnimation_.currentValue().toInt() : heightOfChilds;

            expandingAnimation_.stop();
            expandingAnimation_.setStartValue(0);
            expandingCurrentHeight_ = newCurValue;
            expandingAnimation_.setEndValue(newCurValue);
            expandingAnimation_.setDuration(kExpandingAnimationDuration * progress);
            expandingAnimation_.start();
        }
    }*/
}

void ExpandableItemsWidget::setupExpandingAnimation(QAbstractAnimation::Direction direction, int startValue, int endValue, int duration)
{
    const QSignalBlocker blocker(expandingAnimation_);
    expandingAnimation_.setDirection(direction);
    expandingAnimation_.setStartValue(startValue);
    expandingAnimation_.setEndValue(endValue);
    expandingAnimation_.setDuration(duration);
}


} // namespace gui_locations


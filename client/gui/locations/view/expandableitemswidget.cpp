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
        if (!parent.isValid()) {
            for (int i = first; i <= last; ++i) {
                QModelIndex mi = model_->index(i, 0);
                items_.insert(i, mi);
                itemsCacheData_[mi] = QSharedPointer<IItemCacheData>(delegateForItem(mi)->createCacheData(mi));
                initCacheDataForChilds(mi);
            }
        } else {
            Q_ASSERT(items_.indexOf(parent) != -1);
            initCacheDataForChilds(parent);
        }
        updateExpandingAnimationParams();
        updateHeight();
        update();
    });

    connect(model_, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex &parent, int first, int last) {
        if (!parent.isValid()) {
            for (int i = first; i <= last; ++i)
                clearCacheDataForChilds(model_->index(i, 0));
        }
        else {
            for (int i = first; i <= last; ++i) {
                QPersistentModelIndex childInd = model_->index(i, 0, parent);
                itemsCacheData_.remove(childInd);
            }
        }
    });
    connect(model_, &QAbstractItemModel::rowsRemoved, [this](const QModelIndex &parent, int first, int last) {
        if (!parent.isValid()) {
            for (int i = first; i <= last; ++i) {
                itemsCacheData_.remove(items_[i]);
            }
            Q_ASSERT(last >= (first));
            items_.remove(first, last - first + 1);
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
        for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
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

void ExpandableItemsWidget::updateSelectedItem()
{
    QPoint localCursorPos = mapFromGlobal(QCursor::pos());
    QPersistentModelIndex sel = detectSelectedItem(localCursorPos, &selectedIndRect_);
    if (sel != selectedInd_)
    {
        // close tooltips for prev selected item
        if (selectedInd_.isValid())
            closeAndClearAllActiveTooltips(selectedInd_);

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

    // update tooltips
    if (selectedInd_.isValid()) {
        ItemStyleOption opt(this, selectedIndRect_, 1.0, 0, isShowLocationLoad_, isShowLatencyInMs_);
        int tooltipId = delegateForItem(selectedInd_)->isInTooltipArea(opt, selectedInd_, localCursorPos, itemsCacheData_[selectedInd_].get());
        if (tooltipId == (int)TooltipRect::kNone) {
            closeAndClearAllActiveTooltips(selectedInd_);
        } else {

            // no need to do an increment here
            for (auto it = hoveringToolTips_.begin(); it != hoveringToolTips_.end(); /*it++*/) {
                if (*it != (int)tooltipId) {
                    delegateForItem(selectedInd_)->tooltipLeaveEvent(*it);
                    it = hoveringToolTips_.erase(it);
                } else {
                    ++it;
                }
             }

            if (!hoveringToolTips_.contains(tooltipId)) {
                delegateForItem(selectedInd_)->tooltipEnterEvent(opt, selectedInd_, tooltipId, itemsCacheData_[selectedInd_].get());
                hoveringToolTips_.insert(tooltipId);
            }
        }
    }
}

void ExpandableItemsWidget::paintEvent(QPaintEvent *event)
{
    QElapsedTimer elapsed;
    elapsed.start();

    QPainter painter(this);

    int topOffs = 0;
    for (const auto &it : qAsConst(items_)) {
        IItemDelegate *delegate = delegateForItem(it);
        QRect rcItem(0, topOffs, size().width(), itemHeight_);

        bool isExpanded = expandedItems_.contains(it);
        double expandedProgress;
        if (it == expandingItem_)
            expandedProgress = (double)expandingCurrentHeight_ / (double)expandingAnimation_.endValue().toInt();
        else
            expandedProgress = isExpanded ? 1.0 : 0.0;

        if (rcItem.intersects(event->rect())) {
            ItemStyleOption opt(this, rcItem, it == selectedInd_ ? 1.0 : 0.0, expandedProgress, isShowLocationLoad_, isShowLatencyInMs_);
            delegate->paint(&painter, opt, it, itemsCacheData_[it].get());
        }
        topOffs += itemHeight_;

        // draw child items if this item is expanded
        if (isExpanded) {
            if (it == expandingItem_)
                painter.setClipRect(QRect(0, topOffs, size().width(), expandingCurrentHeight_));
            int row = 0;
            int overallHeight = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid()) {
                QRect rcChildItem(0, topOffs + overallHeight, size().width(), itemHeight_);
                if (rcChildItem.intersects(event->rect())) {
                    ItemStyleOption opt(this, rcChildItem, childInd == selectedInd_ ? 1.0 : 0.0, false, isShowLocationLoad_, isShowLatencyInMs_);
                    nonexpandableItemDelegate_->paint(&painter, opt, childInd, itemsCacheData_[childInd].get());
                }
                overallHeight += itemHeight_;
                row++;
                childInd = it.model()->index(row, 0, it);
            }

            if (it == expandingItem_) {
                painter.setClipping(false);
                topOffs += expandingCurrentHeight_;
            }
            else {
                topOffs += overallHeight;
            }
        }
    }
    g_time += elapsed.elapsed();
    g_cntFrames++;
}

void ExpandableItemsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (bMousePressed_)
        return;
    updateSelectedItem();
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
            ItemStyleOption opt(this, rcItem, 1.0, 0, isShowLocationLoad_, isShowLatencyInMs_);
            mousePressedClickableId_ = delegateForItem(mousePressedItem_)->isInClickableArea(opt, mousePressedItem_, event->pos());
            bMousePressed_ = true;
        }
    }

    QWidget::mousePressEvent(event);
}

void ExpandableItemsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && bMousePressed_) {
        QRect rcItem;
        if (mousePressedItem_ == detectSelectedItem(event->pos(), &rcItem)) {
            if (isExpandableItem(mousePressedItem_)) {
                if (expandingAnimation_.state() != QAbstractAnimation::Running) {

                    if (expandedItems_.contains(mousePressedItem_))
                    {
                        expandingItem_ = mousePressedItem_;
                        expandingCurrentHeight_ = calcHeightOfChildItems(expandingItem_);
                        setupExpandingAnimation(QAbstractAnimation::Backward, 0, expandingCurrentHeight_, kExpandingAnimationDuration);
                        emit expandingAnimationStarted(0, 0);        // top, bottom values are not needed when collapsing
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
                        int offs = getOffsForItem(expandingItem_);
                        emit expandingAnimationStarted(offs, heightOfChilds);
                        expandingAnimation_.start();
                    }

                    updateHeight();
                    update();
                }
            }
            else    // non expandable item
            {
                // todo city click
                ItemStyleOption opt(this, rcItem, 1.0, 0, isShowLocationLoad_, isShowLatencyInMs_);
                int clickableId = delegateForItem(mousePressedItem_)->isInClickableArea(opt, mousePressedItem_, event->pos());
                if (clickableId != -1 && clickableId == mousePressedClickableId_)
                {
                    if (clickableId == (int)ClickableRect::kFavorite)
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

void ExpandableItemsWidget::leaveEvent(QEvent *event)
{
    if (selectedInd_.isValid())
        closeAndClearAllActiveTooltips(selectedInd_);
    QWidget::leaveEvent(event);
}

void ExpandableItemsWidget::onExpandingAnimationValueChanged(const QVariant &value)
{
    expandingCurrentHeight_ = value.toInt();
    updateHeight();
    update();

    // Makes sense only for expanding.
    if (expandingAnimation_.direction() == QAbstractAnimation::Forward) {
        emit expandingAnimationProgress((qreal)expandingAnimation_.currentTime() / (qreal)expandingAnimation_.duration());
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
    } else {
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
    if (expandingAnimation_.state() != QAbstractAnimation::Running)
        return;

    // check if expanding item is deleted
    if (!expandingItem_.isValid()) {
        expandingAnimation_.stop();
        expandingItem_ = QModelIndex();
        return;
    }

    int endValue = expandingAnimation_.endValue().toInt();
    int heightOfChilds = calcHeightOfChildItems(expandingItem_);
    if (heightOfChilds != endValue) {
        int curTime = expandingAnimation_.currentTime();

        if (expandingAnimation_.direction() == QAbstractAnimation::Forward) {
            int curValue = expandingAnimation_.currentValue().toInt();
            expandingAnimation_.stop();

            if (curValue >= heightOfChilds) {
                expandingItem_ = QModelIndex();
            } else {
                expandingCurrentHeight_ = curValue;
                setupExpandingAnimation(QAbstractAnimation::Forward, curValue, heightOfChilds, kExpandingAnimationDuration - curTime);

                int offs = getOffsForItem(expandingItem_);
                emit expandingAnimationStarted(offs, heightOfChilds);

                expandingAnimation_.start();
            }
        } else { // QAbstractAnimation::Backward
            int newCurValue = expandingAnimation_.currentValue().toInt() < heightOfChilds ? expandingAnimation_.currentValue().toInt() : heightOfChilds;

            expandingAnimation_.stop();
            expandingCurrentHeight_ = newCurValue;
            setupExpandingAnimation(QAbstractAnimation::Backward, 0, newCurValue, kExpandingAnimationDuration - curTime);
            expandingAnimation_.start();
        }
    }
}

void ExpandableItemsWidget::setupExpandingAnimation(QAbstractAnimation::Direction direction, int startValue, int endValue, int duration)
{
    const QSignalBlocker blocker(expandingAnimation_);
    expandingAnimation_.setDirection(direction);
    expandingAnimation_.setStartValue(startValue);
    expandingAnimation_.setEndValue(endValue);
    expandingAnimation_.setDuration(duration);
}

void ExpandableItemsWidget::closeAndClearAllActiveTooltips(const QPersistentModelIndex &modelIndex)
{
    Q_ASSERT(modelIndex.isValid());
    for (auto id : qAsConst(hoveringToolTips_)) {
        delegateForItem(modelIndex)->tooltipLeaveEvent(id);
    }
    hoveringToolTips_.clear();
}


} // namespace gui_locations


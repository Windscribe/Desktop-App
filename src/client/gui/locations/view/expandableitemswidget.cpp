#include "expandableitemswidget.h"

#include <QCursor>
#include <QDebug>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QRegion>

#include "dpiscalemanager.h"
#include "locations/locationsmodel_roles.h"
#include "clickableandtooltiprects.h"
#include "commongraphics/commongraphics.h"

namespace gui_locations {

ExpandableItemsWidget::ExpandableItemsWidget(QWidget *parent, QAbstractItemModel *model) : QWidget(parent)
  , cursorUpdateHelper_(new CursorUpdateHelper(this))
  , isEmptyList_(true)
  , itemHeight_(0)
  , isShowLocationLoad_(false)
  , isShowCountryFlagForCity_(false)
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
}

void ExpandableItemsWidget::setModel(QAbstractItemModel *model)
{
    WS_ASSERT(model_ == nullptr);
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
            WS_ASSERT(items_.indexOf(parent) != -1);
            initCacheDataForChilds(parent);
        }
        updateExpandingAnimationParams();
        updateHeight();
        update();
        debugAssertCheckInternalData();
    });

    connect(model_, &QAbstractItemModel::rowsRemoved, [this](const QModelIndex &parent, int first, int last) {
        removeCacheDataForInvalidIndexes();
        removeInvalidExpandedIndexes();

        if (!parent.isValid()) {
            WS_ASSERT(last >= (first));
            items_.remove(first, last - first + 1);
        }
        updateExpandingAnimationParams();
        updateHeight();
        update();
        debugAssertCheckInternalData();
    });

    connect(model_, &QAbstractItemModel::rowsMoved, [this]() {
        WS_ASSERT(false);  // not supported
    });

    connect(model_, &QAbstractItemModel::dataChanged, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
        if (!topLeft.isValid() || !bottomRight.isValid()) {
            return;
        }
        WS_ASSERT(topLeft.parent() == bottomRight.parent());
        // update cache data
        for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
            QPersistentModelIndex mi = model_->index(i, 0, topLeft.parent());
            WS_ASSERT(mi.isValid());
            WS_ASSERT(itemsCacheData_.contains(mi));
            delegateForItem(mi)->updateCacheData(mi, itemsCacheData_[mi].get());
        }
        update();
    });

    connect(model_, &QAbstractItemModel::layoutChanged, [this]() {
        removeCacheDataForInvalidIndexes();
        removeInvalidExpandedIndexes();
        items_.clear();
        for (int i = 0, rows_cnt = model_->rowCount(); i < rows_cnt; ++i) {
            QModelIndex mi = model_->index(i, 0);
            items_ << mi;
            if (!itemsCacheData_.contains(mi))
                itemsCacheData_[mi] = QSharedPointer<IItemCacheData>(delegateForItem(mi)->createCacheData(mi));

            initCacheDataForChilds(mi);
        }

        updateExpandingAnimationParams();
        updateHeight();
        update();
        debugAssertCheckInternalData();
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

void ExpandableItemsWidget::setShowLocationLoad(bool isShowLocationLoad)
{
    isShowLocationLoad_ = isShowLocationLoad;
    update();
}

void ExpandableItemsWidget::updateSelectedItemByCursorPos()
{
    QPoint localCursorPos = mapFromGlobal(QCursor::pos());
    QPersistentModelIndex sel = detectSelectedItem(localCursorPos, &selectedIndRect_);
    if (sel != selectedInd_)
    {
        // close tooltips for prev selected item
        if (selectedInd_.isValid()) {
            closeAndClearAllActiveTooltips(selectedInd_);
        }

        selectedInd_ = sel;
    }

    // update cursor
    if (selectedInd_.isValid()) {
        if (selectedInd_.isValid() && delegateForItem(selectedInd_)->isForbiddenCursor(selectedInd_)) {
            cursorUpdateHelper_->setForbiddenCursor();
        } else {
            cursorUpdateHelper_->setPointingHandCursor();
        }
        update();
    }

    // update tooltips
    if (selectedInd_.isValid()) {
        ItemStyleOption opt(this, selectedIndRect_, 1.0, 0, isShowLocationLoad_, isShowCountryFlagForCity_);
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

int ExpandableItemsWidget::selectItemByOffs(int offs)
{
    // if no selected item, then select first one
    if (!selectedInd_.isValid()) {
        if (items_.count() > 0) {
            selectedInd_ = items_[0];
            update();
        }
    } else {
        QVector<ItemRect> items = getItemRects();
        auto it = std::find_if(items.begin(), items.end(), [this](const ItemRect &item) { return item.modelIndex == selectedInd_;});
        if (it == items.end()) {
            WS_ASSERT(false);
            return 0;
        } else {
            int newSelectedItemInd = it - items.begin()  + offs;
            newSelectedItemInd = qMin(newSelectedItemInd, items.count() - 1);
            newSelectedItemInd = qMax(newSelectedItemInd, 0);
            selectedInd_ = items[newSelectedItemInd].modelIndex;
            if (selectedInd_.isValid() && delegateForItem(selectedInd_)->isForbiddenCursor(selectedInd_)) {
                cursorUpdateHelper_->setForbiddenCursor();
            } else {
                cursorUpdateHelper_->setPointingHandCursor();
            }
            update();
            return newSelectedItemInd * itemHeight_;
        }
    }
    return 0;
}

void ExpandableItemsWidget::expandAll()
{
    stopExpandingAnimation();
    expandedItems_.clear();
    for (const auto &it : items_)
        expandedItems_.insert(it);

    updateHeight();
    update();
}

void ExpandableItemsWidget::collapseAll()
{
    stopExpandingAnimation();
    expandedItems_.clear();
    updateHeight();
    update();
}

void ExpandableItemsWidget::doActionOnSelectedItem()
{
    if (!selectedInd_.isValid())
        return;

    if (isExpandableItem(selectedInd_) && model_->rowCount(selectedInd_) > 0) {
        expandItem(selectedInd_);
    }
    else {
        if (!delegateForItem(selectedInd_)->isForbiddenCursor(selectedInd_)) {
            emit selected(qvariant_cast<LocationID>(selectedInd_.data(gui_locations::kLocationId)));
        }
        else if (selectedInd_.data(kIsShowAsPremium).toBool()) {
            emit clickedOnPremiumStarCity();
        }
    }
}

void ExpandableItemsWidget::updateScaling()
{
    int newHeight = qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    if (itemHeight_ != newHeight) {
        itemHeight_ = newHeight;
        updateHeight();
    }

    itemsCacheData_.clear();
    for (int i = 0, rows_cnt = model_->rowCount(); i < rows_cnt; ++i) {
        QModelIndex mi = model_->index(i, 0);
        itemsCacheData_[mi] = QSharedPointer<IItemCacheData>(delegateForItem(mi)->createCacheData(mi));
        initCacheDataForChilds(mi);
    }
    update();
}

void ExpandableItemsWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QVector<ItemRect> items = getItemRects();
    QPoint localCursorPos = mapFromGlobal(QCursor::pos());

    for (const auto &item : std::as_const(items)) {
        IItemDelegate *delegate = delegateForItem(item.modelIndex);
        double expandedProgress;
        if (item.modelIndex == expandingItem_)
            expandedProgress = (double)expandingCurrentHeight_ / (double)expandingAnimation_.endValue().toInt();
        else
            expandedProgress = item.isExpanded ? 1.0 : 0.0;

        if (item.rc.intersects(event->rect())) {
            QRect fullItemRect(item.rc.left(), item.rc.top(), item.rc.width(), itemHeight_);

            // Determine which clickable area is being hovered
            int hoverClickableId = -1;
            if (item.modelIndex == selectedInd_) {
                ItemStyleOption tempOpt(this, fullItemRect, 1.0, expandedProgress, isShowLocationLoad_, isShowCountryFlagForCity_);
                hoverClickableId = delegate->isInClickableArea(tempOpt, item.modelIndex, localCursorPos);
            }

            ItemStyleOption opt(this, fullItemRect, item.modelIndex == selectedInd_ ? 1.0 : 0.0, expandedProgress, isShowLocationLoad_, isShowCountryFlagForCity_, hoverClickableId);
            delegate->paint(&painter, opt, item.modelIndex, itemsCacheData_[item.modelIndex].get());
        }
    }
}

void ExpandableItemsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (bMousePressed_)
        return;
    updateSelectedItemByCursorPos();
    QWidget::mouseMoveEvent(event);
}

void ExpandableItemsWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QRect rcItem;
        mousePressedItem_ = detectSelectedItem(event->pos(), &rcItem);
        if (mousePressedItem_.isValid()) {
            ItemStyleOption opt(this, rcItem, 1.0, 0, isShowLocationLoad_, isShowCountryFlagForCity_);
            mousePressedClickableId_ = delegateForItem(mousePressedItem_)->isInClickableArea(opt, mousePressedItem_, event->pos());

            if (isExpandableItem(mousePressedItem_) && model_->rowCount(mousePressedItem_) > 0) {
                expandItem(mousePressedItem_);
            }
            else {
                bMousePressed_ = true;
            }
        }
    }

    QWidget::mousePressEvent(event);
}

void ExpandableItemsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && bMousePressed_) {
        QRect rcItem;
        if (mousePressedItem_ == detectSelectedItem(event->pos(), &rcItem)) {
            /*if (isExpandableItem(mousePressedItem_) && model_->rowCount(mousePressedItem_) == 0) {
                // Best location click
                LocationID lid = qvariant_cast<LocationID>(mousePressedItem_.data(gui_locations::kLocationId));
                WS_ASSERT(lid.isBestLocation());
                emit selected(lid);
            }
            else*/ if (isExpandableItem(mousePressedItem_) && model_->rowCount(mousePressedItem_) > 0) {
                WS_ASSERT(false);    // The expanding/collapsing must have already been processed in mousePressEvent
            }
            else    // non expandable item
            {
                // todo city click
                ItemStyleOption opt(this, rcItem, 1.0, 0, isShowLocationLoad_, isShowCountryFlagForCity_);
                int clickableId = delegateForItem(mousePressedItem_)->isInClickableArea(opt, mousePressedItem_, event->pos());
                if (clickableId != -1 && clickableId == mousePressedClickableId_)
                {
                    if (clickableId == (int)ClickableRect::kFavorite)
                    {
                        model_->setData(mousePressedItem_, !mousePressedItem_.data(kIsFavorite).toBool(), kIsFavorite);
                    }
                } else if (clickableId == -1 && !delegateForItem(mousePressedItem_)->isForbiddenCursor(mousePressedItem_)) {
                    emit selected(qvariant_cast<LocationID>(mousePressedItem_.data(gui_locations::kLocationId)));
                }
                else if (clickableId == -1 && mousePressedItem_.data(kIsShowAsPremium).toBool()) {
                    emit clickedOnPremiumStarCity();
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
    if (selectedInd_.isValid()) {
        closeAndClearAllActiveTooltips(selectedInd_);
        // Clear hover state for selected item now that the mouse cursor has left the locations list.
        selectedInd_ = QPersistentModelIndex();
        update();
    }

    QWidget::leaveEvent(event);
}

void ExpandableItemsWidget::onExpandingAnimationValueChanged(const QVariant &value)
{
    expandingCurrentHeight_ = value.toInt();
    updateHeight();
    update();
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
    stopExpandingAnimation();
    selectedInd_ = QModelIndex();

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
    debugAssertCheckInternalData();
}

QPersistentModelIndex ExpandableItemsWidget::detectSelectedItem(const QPoint &pt, QRect *outputRect)
{
    QVector<ItemRect> items = getItemRects();
    for (const auto &item : std::as_const(items)) {
        if (item.rc.contains(pt)) {
            if (outputRect) {
                *outputRect = item.rc;
            }
            return item.modelIndex;
        }
    }

    return QPersistentModelIndex();
}

IItemDelegate *ExpandableItemsWidget::delegateForItem(const QPersistentModelIndex &ind)
{
    // Is it an expandable item?
    if (isExpandableItem(ind))
        return expandableItemDelegate_;
    else
        return nonexpandableItemDelegate_;
}

bool ExpandableItemsWidget::isExpandableItem(const QPersistentModelIndex &ind)
{
    return ind.data(gui_locations::kIsTopLevelLocation).toBool();
}

void ExpandableItemsWidget::updateHeight()
{
    int curHeight = 0;
    for (const auto &it : std::as_const(items_)) {
        curHeight += itemHeight_;
        if (expandedItems_.contains(it)) {
            if (it == expandingItem_)
                curHeight += expandingCurrentHeight_;
            else
                curHeight += calcHeightOfChildItems(it);
        }
    }
    resize(size().width(), curHeight);

    if (curHeight == 0) {
        if (isEmptyList_ == false) {
            isEmptyList_ = true;
            emit emptyListStateChanged(true);
        }
    } else {
        if (isEmptyList_ == true) {
            isEmptyList_ = false;
            emit emptyListStateChanged(false);
        }
    }
}

int ExpandableItemsWidget::calcHeightOfChildItems(const QPersistentModelIndex &ind)
{
    return model_->rowCount(ind) * itemHeight_;
}

int ExpandableItemsWidget::getOffsForTopLevelItem(const QPersistentModelIndex &ind)
{
    int top = 0;
    for (const auto &it : std::as_const(items_)) {
        if (it == ind)
            return top;
        top += itemHeight_;
        if (expandedItems_.contains(it))
            top += model_->rowCount(it) * itemHeight_;
    }
    WS_ASSERT(false);
    return 0;
}

void ExpandableItemsWidget::initCacheDataForChilds(const QPersistentModelIndex &parentInd)
{
    for (int c = 0, childs_cnt = model_->rowCount(parentInd); c < childs_cnt; ++c )
    {
        QModelIndex childMi = model_->index(c, 0, parentInd);
        if (!itemsCacheData_.contains(childMi)) {
            itemsCacheData_[childMi] = QSharedPointer<IItemCacheData>(delegateForItem(childMi)->createCacheData(childMi));
        }
    }
}

void ExpandableItemsWidget::clearCacheDataForChilds(const QPersistentModelIndex &parentInd)
{
    for (int c = 0, childs_cnt = model_->rowCount(parentInd); c < childs_cnt; ++c )
    {
        QModelIndex childMi = model_->index(c, 0, parentInd);
        auto it = itemsCacheData_.constFind(childMi);
        WS_ASSERT(it != itemsCacheData_.end());
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

                int offs = getOffsForTopLevelItem(expandingItem_);
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
    WS_ASSERT(modelIndex.isValid());
    for (auto id : std::as_const(hoveringToolTips_)) {
        delegateForItem(modelIndex)->tooltipLeaveEvent(id);
    }
    hoveringToolTips_.clear();
}

void ExpandableItemsWidget::removeInvalidExpandedIndexes()
{
    // remove expanded items for invalid indexes
    for (auto it = expandedItems_.begin(); it != expandedItems_.end(); /*it++*/) {
        if (!it->isValid())
            it = expandedItems_.erase(it);
        else
            ++it;
    }
}

void ExpandableItemsWidget::removeCacheDataForInvalidIndexes()
{
    // remove cache data for invalid indexes
    for (auto it = itemsCacheData_.begin(); it != itemsCacheData_.end(); /*it++*/) {
        if (!it.key().isValid())
            it = itemsCacheData_.erase(it);
        else
            ++it;
    }
}

void ExpandableItemsWidget::debugAssertCheckInternalData()
{
#ifdef QT_DEBUG
    int cnt = 0;
    for (int r = 0, cntRows = model_->rowCount(); r < cntRows; ++r) {
        QModelIndex parentMi = model_->index(r, 0);
        WS_ASSERT(itemsCacheData_.contains(parentMi));
        cnt++;

        for (int c = 0, cntChildRows = model_->rowCount(parentMi); c < cntChildRows; ++c) {
            QModelIndex childMi = model_->index(c, 0, parentMi);
            WS_ASSERT(itemsCacheData_.contains(childMi));
            cnt++;
        }
    }
    WS_ASSERT(cnt == itemsCacheData_.count());

    for (const auto &it : items_) {
        WS_ASSERT(it.isValid());
    }

    for (const auto &it : expandedItems_) {
        WS_ASSERT(it.isValid());
    }

    if (expandingAnimation_.state() == QAbstractAnimation::Running)
    {
        WS_ASSERT(expandingItem_.isValid());
    }

#endif
}

void ExpandableItemsWidget::stopExpandingAnimation()
{
    if (expandingAnimation_.state() == QAbstractAnimation::Running) {
        expandingAnimation_.stop();
        expandingItem_ = QModelIndex();
    }
}

void ExpandableItemsWidget::expandItem(const QPersistentModelIndex &ind)
{
    WS_ASSERT(isExpandableItem(ind));
    if (model_->rowCount(ind) == 0) // is item has children?
        return;

    if (expandingAnimation_.state() != QAbstractAnimation::Running) {
        if (expandedItems_.contains(ind)) {
            expandingItem_ = ind;
            expandingCurrentHeight_ = calcHeightOfChildItems(expandingItem_);
            setupExpandingAnimation(QAbstractAnimation::Backward, 0, expandingCurrentHeight_, kExpandingAnimationDuration);
            emit expandingAnimationStarted(0, 0);        // top, bottom values are not needed when collapsing
            expandingAnimation_.start();
        } else {
            isAnimationJustStarted_ = true;
            expandedItems_.insert(ind);
            expandingItem_ = ind;
            expandingCurrentHeight_ = 0;
            int heightOfChilds = calcHeightOfChildItems(expandingItem_);
            setupExpandingAnimation(QAbstractAnimation::Forward, 0, heightOfChilds, kExpandingAnimationDuration);
            int offs = getOffsForTopLevelItem(expandingItem_);
            emit expandingAnimationStarted(offs, heightOfChilds);
            expandingAnimation_.start();
        }
        updateHeight();
        update();
    }
}

QVector<ExpandableItemsWidget::ItemRect> ExpandableItemsWidget::getItemRects()
{
    QVector<ItemRect> result;
    result.reserve(items_.count() * 3);
    int top = 0;

    for (const auto &it : std::as_const(items_)) {
        bool isExpandedItem = expandedItems_.contains(it);
        QRect rcItem = QRect(0, top, size().width(), itemHeight_);
        result << ItemRect{it, rcItem, isExpandedItem};
        top += itemHeight_;

        if (isExpandedItem) {
            int childsCount = it.model()->rowCount(it);
            int expandedHeight = (it == expandingItem_) ? expandingCurrentHeight_:  childsCount * itemHeight_;

            for (int row = 0; row < childsCount; row++)
            {
                QModelIndex childInd = it.model()->index(row, 0, it);
                int curItemHeight = expandedHeight >= itemHeight_ ? itemHeight_ : expandedHeight;
                if (curItemHeight == 0)
                    break;

                QRect rcChildItem(0, top, size().width(), curItemHeight);
                result << ItemRect{childInd, rcChildItem, false};

                top += curItemHeight;
                expandedHeight -= curItemHeight;
                if (expandedHeight == 0)
                    break;
                if (expandedHeight < 0)
                    WS_ASSERT(false);
            }
        }
    }
    return result;
}

int ExpandableItemsWidget::count() const
{
    int count = 0;

    for (const auto &it : std::as_const(items_)) {
        count += it.model()->rowCount(it);
    }

    // If 0, return the number of top level items instead
    if (count == 0) {
        count = items_.count();
    }

    return count;
}

void ExpandableItemsWidget::setShowCountryFlagForCity(bool show)
{
    isShowCountryFlagForCity_ = show;
}

} // namespace gui_locations

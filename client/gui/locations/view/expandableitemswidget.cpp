#include "expandableitemswidget.h"
#include "clickableandtooltiprects.h"
#include "../locationsmodel_roles.h"

#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QRegion>
#include <QElapsedTimer>

long g_cntFrames = 0;
qint64 g_time = 0;

namespace gui_locations {

ExpandableItemsWidget::ExpandableItemsWidget(QWidget *parent) : QWidget(parent)
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
    connect(&expandingAnimation_, &QVariantAnimation::valueChanged, this, &ExpandableItemsWidget::onExpandingAnimationValueChanged);
    connect(&expandingAnimation_, &QVariantAnimation::finished, this, &ExpandableItemsWidget::onExpandingAnimationFinished);
}

ExpandableItemsWidget::~ExpandableItemsWidget()
{
    qDebug() << (double)g_time / (double)g_cntFrames;
}

void ExpandableItemsWidget::setModel(QAbstractItemModel *model)
{
    Q_ASSERT(model_ == nullptr);    // currently, the setup of the model is supported only once
    model_ = model;

    connect(model_, &QAbstractItemModel::modelReset, [=]()  { resetItemsList();  });

    /*connect(model_, &QAbstractItemModel::rowsInserted,
                [=]()
        {
            //updateItemsList();
        });

    connect(model_, &QAbstractItemModel::rowsRemoved,
                [=]()
        {
            //updateItemsList();
        });

    connect(model_, &QAbstractItemModel::rowsMoved, [=]() { Q_ASSERT(false); });

    connect(model_, &QAbstractItemModel::dataChanged,
                [=]()
        {
            update();
        });

    connect(model_, &QAbstractItemModel::layoutChanged,
                [=]()
        {
            sortItemsListByRow();
            update();
        });
    */

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
            expandedProgress = (double)expandingAnimation_.currentTime() / (double)expandingAnimation_.totalDuration();
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

                    if (expandedItems_.contains(mousePressedItem_))
                    {
                        expandingItem_ = mousePressedItem_;
                        expandingCurrentHeight_ = calcHeightOfChildItems(expandingItem_);
                        expandingAnimation_.setDuration(EXPANDING_ANIMATION_DURATION);
                        expandingAnimation_.setStartValue(0);
                        expandingAnimation_.setEndValue(expandingCurrentHeight_);
                        expandingAnimation_.setDirection(QAbstractAnimation::Backward);
                        expandingAnimation_.start();
                    }
                    else
                    {
                        expandedItems_.insert(mousePressedItem_);
                        expandingItem_ = mousePressedItem_;
                        expandingCurrentHeight_ = 0;
                        int heightOfChilds = calcHeightOfChildItems(expandingItem_);
                        expandingAnimation_.setDuration(EXPANDING_ANIMATION_DURATION);
                        expandingAnimation_.setStartValue(0);
                        expandingAnimation_.setEndValue(heightOfChilds);
                        expandingAnimation_.setDirection(QAbstractAnimation::Forward);
                        expandingAnimation_.start();

                        int offs = getOffsForItem(expandingItem_);
                        emit notifyMustBeVisible(offs, offs +  model_->rowCount(expandingItem_));
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
                        model_->setData(mousePressedItem_, !mousePressedItem_.data(IS_FAVORITE).toBool(), IS_FAVORITE);
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
    //emit notifyExpandingAnimationFinished();
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

        for (int c = 0, childs_cnt = model_->rowCount(mi); c < childs_cnt; ++c )
        {
            QModelIndex childMi = model_->index(c, 0, mi);
            itemsCacheData_[childMi] = QSharedPointer<IItemCacheData>(delegateForItem(childMi)->createCacheData(childMi));
        }
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
    return ind.data(gui_locations::IS_TOP_LEVEL_LOCATION).toBool();
}

void ExpandableItemsWidget::updateHeight()
{
    int curHeight = 0;
    for (const auto &it : qAsConst(items_))
    {
        curHeight += itemHeight_;
        if (expandedItems_.contains(it))
        {
            if (it == expandingItem_)
            {
                curHeight += expandingCurrentHeight_;
            }
            else
            {
                curHeight += calcHeightOfChildItems(it);
            }
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
    for (const auto &it : qAsConst(items_))
    {
        if (it == ind)
        {
            return top;
        }
        top++;
        if (expandedItems_.contains(it))
        {
            top += model_->rowCount(it);
        }
    }
}


} // namespace gui_locations


#include "expandableitemswidget.h"
#include "clickableandtooltiprects.h"
#include "../locationsmodel_roles.h"

#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QRegion>

namespace gui_locations {

ExpandableItemsWidget::ExpandableItemsWidget(QWidget *parent) : QWidget(parent)
  , cursorUpdateHelper_(new CursorUpdateHelper(this))
  , itemHeight_(0)
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
}

void ExpandableItemsWidget::setModel(QAbstractItemModel *model)
{
    model_ = model;

    // todo remove prev signals
    connect(model_, &QAbstractItemModel::modelReset,
                [=]()
        {
            updateItemsList();
        });

    connect(model_, &QAbstractItemModel::rowsInserted,
                [=]()
        {
            updateItemsList();
        });

    connect(model_, &QAbstractItemModel::rowsRemoved,
                [=]()
        {
            updateItemsList();
        });

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


    updateItemsList();
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

void ExpandableItemsWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QRect bkgd(0, 0, geometry().width(), geometry().height());
    painter.fillRect(bkgd, QColor(255, 255, 0));

    int top = 0;
    for (const auto &it : qAsConst(items_))
    {
        IItemDelegate *delegate = delegateForItem(it);

        QRect rcItem(0, top, size().width(), itemHeight_);

        bool isExpanded = expandedItems_.contains(it);

        double expandedProgress;
        if (it == expandingItem_)
        {
            expandedProgress = (double)expandingAnimation_.currentTime() / (double)expandingAnimation_.totalDuration();
        }
        else
        {
            expandedProgress = isExpanded ? 1.0 : 0.0;
        }

        ItemStyleOption opt(rcItem, it == selectedInd_ ? 1.0 : 0.0, expandedProgress, true, false);
        delegate->paint(&painter, opt, it);
        top += itemHeight_;

        // draw child items if this item is expanded
        if (isExpanded)
        {
            if (it == expandingItem_)
            {
                painter.setClipRect(QRect(0, top, size().width(), expandingCurrentHeight_));
            }
            int row = 0;
            int overallHeight = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid())
            {
                QRect rcChildItem(0, top + overallHeight, size().width(), itemHeight_);

                ItemStyleOption opt(rcChildItem, childInd == selectedInd_ ? 1.0 : 0.0, false, true, false);
                nonexpandableItemDelegate_->paint(&painter, opt, childInd);
                //top += curChildItemHeight;
                overallHeight += itemHeight_;

                row++;
                childInd = it.model()->index(row, 0, it);
            }

            if (it == expandingItem_)
            {
                painter.setClipping(false);
                top += expandingCurrentHeight_;
            }
            else
            {
                top += overallHeight;
            }
        }
    }
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

void ExpandableItemsWidget::updateItemsList()
{
    items_.clear();
    expandedItems_.clear();
    int rows_cnt = model_->rowCount();
    for (int i = 0; i < rows_cnt; ++i)
    {
        QModelIndex mi = model_->index(i, 0);
        items_ << mi;
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
    // Updating only height
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


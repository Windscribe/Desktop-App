#include "expandableitemswidget.h"
#include "clickableandtooltiprects.h"
#include "../locationsmodel_roles.h"

#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

namespace gui_location {

ExpandableItemsWidget::ExpandableItemsWidget(QWidget *parent) : QWidget(parent)
  , cursorUpdateHelper_(new CursorUpdateHelper(this))
  , model_(nullptr)
  , expandableItemDelegate_(nullptr)
  , nonexpandableItemDelegate_(nullptr)
  , curWidth_(0)
  , curHeight_(0)
  , bMousePressed_(false)
  , mousePressedClickableId_(-1)
{
    setMouseTracking(true);
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

void ExpandableItemsWidget::setItemDelegateForExpandableItem(IItemDelegate *itemDelegate)
{
    expandableItemDelegate_ = itemDelegate;
}

void ExpandableItemsWidget::setItemDelegateForNonExpandableItem(IItemDelegate *itemDelegate)
{
    nonexpandableItemDelegate_ = itemDelegate;
}

void ExpandableItemsWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QRect bkgd(0, 0, geometry().width(), geometry().height());
    painter.fillRect(bkgd, QColor(255, 0, 0));

    int top = 0;
    for (auto it : qAsConst(items_))
    {
        IItemDelegate *delegate = delegateForItem(it);

        int curItemHeight = delegate->sizeHint(it).height();
        QRect rcItem(0, top, curWidth_, curItemHeight);

        bool isExpanded = expandedItems_.contains(it);
        ItemStyleOption opt(rcItem, it == selectedInd_ ? 1.0 : 0.0, isExpanded, true, false);
        delegate->paint(&painter, opt, it);
        top += curItemHeight;


        // draw child items if this item is expanded
        if (isExpanded)
        {
            int row = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid())
            {
                int curChildItemHeight = nonexpandableItemDelegate_->sizeHint(childInd).height();
                QRect rcChildItem(0, top, curWidth_, curChildItemHeight);

                ItemStyleOption opt(rcChildItem, childInd == selectedInd_ ? 1.0 : 0.0, false, true, false);
                nonexpandableItemDelegate_->paint(&painter, opt, childInd);
                top += curChildItemHeight;

                row++;
                childInd = it.model()->index(row, 0, it);
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
        mousePressedInd_ = detectSelectedItem(event->pos(), &rcItem);
        if (mousePressedInd_.isValid())
        {
            mousePressedClickableId_ = delegateForItem(mousePressedInd_)->isInClickableArea(mousePressedInd_, event->pos() - rcItem.topLeft());
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
        if (mousePressedInd_ == detectSelectedItem(event->pos(), &rcItem))
        {
            if (isExpandableItem(mousePressedInd_))
            {
                if (expandedItems_.contains(mousePressedInd_))
                {
                    expandedItems_.remove(mousePressedInd_);
                }
                else
                {
                    // if the item has childs
                    if (mousePressedInd_.model()->index(0, 0, mousePressedInd_).isValid())
                    {
                        expandedItems_.insert(mousePressedInd_);
                    }
                }
                updateWidgetSize();
                update();
            }
            else    // non expandable item
            {
                // todo city click
                int clickableId = delegateForItem(mousePressedInd_)->isInClickableArea(mousePressedInd_, event->pos() - rcItem.topLeft());
                if (clickableId != -1 && clickableId == mousePressedClickableId_)
                {
                    if (clickableId == CLICKABLE_FAVORITE_RECT)
                    {
                        model_->setData(mousePressedInd_, !mousePressedInd_.data(IS_FAVORITE).toBool(), IS_FAVORITE);
                    }
                }
            }
        }
        bMousePressed_ = false;
        mouseMoveEvent(event);  // detect selected item
    }

    QWidget::mouseReleaseEvent(event);
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

    updateWidgetSize();
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
        int curItemHeight = delegateForItem(it)->sizeHint(it).height();
        QRect rcItem = QRect(0, top, curWidth_, curItemHeight);
        if (rcItem.contains(pt))
        {
            if (outputRect) {
                *outputRect = rcItem;
            }
            return it;
        }
        top += curItemHeight;

        if (expandedItems_.contains(it))
        {
            int row = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid())
            {
                int curChildItemHeight = nonexpandableItemDelegate_->sizeHint(childInd).height();
                QRect rcChildItem(0, top, curWidth_, curChildItemHeight);
                if (rcChildItem.contains(pt))
                {
                    if (outputRect) {
                        *outputRect = rcChildItem;
                    }
                    return childInd;
                }
                top += curChildItemHeight;
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
    // Qt::ItemNeverHasChildren flag must be set in the model for non expandable items
    if ((ind.flags() != Qt::ItemNeverHasChildren)) {
        return expandableItemDelegate_;
    }
    // a non expandable item
    else {
        return nonexpandableItemDelegate_;
    }
}

bool ExpandableItemsWidget::isExpandableItem(const QPersistentModelIndex &ind)
{
    return ind.flags() != Qt::ItemNeverHasChildren;
}

void ExpandableItemsWidget::updateWidgetSize()
{
    curWidth_ = 0;
    curHeight_ = 0;

    for (auto it : qAsConst(items_))
    {
        QSize sz = delegateForItem(it)->sizeHint(it);
        int curItemHeight = sz.height();
        curHeight_ += curItemHeight;
        curWidth_ = sz.width();

        if (expandedItems_.contains(it))
        {
            int row = 0;
            QModelIndex childInd = it.model()->index(row, 0, it);
            while (childInd.isValid())
            {
                int curChildItemHeight = nonexpandableItemDelegate_->sizeHint(childInd).height();
                curHeight_ += curChildItemHeight;
                row++;
                childInd = it.model()->index(row, 0, it);
            }
        }
    }
    resize(curWidth_, curHeight_);
}





} // namespace gui_location


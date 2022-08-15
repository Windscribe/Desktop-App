#ifndef GUI_LOCATIONS_IITEMDELEGATE_H
#define GUI_LOCATIONS_IITEMDELEGATE_H

#include <QWidget>
#include <QAbstractItemModel>
#include <QStyleOption>

namespace gui_location {

class ItemStyleOption : public QStyleOption
{
public:
    explicit ItemStyleOption(const QRect &rc, double selectedOpacity, double expandedProgress, bool isShowLocationLoad, bool isShowLatencyInMs) :
        selectedOpacity_(selectedOpacity),
        expandedProgress_(expandedProgress),
        isShowLocationLoad_(isShowLocationLoad),
        isShowLatencyInMs_(isShowLatencyInMs)
    {
        rect = rc;
    }

    // selected [0..1] -> opacity of the selection
    double selectedOpacity() const
    {
        return selectedOpacity_;
    }

    // Expanded animation progress [0..1]
    double expandedProgress() const
    {
        return expandedProgress_;
    }

    bool isShowLocationLoad() const
    {
        return isShowLocationLoad_;
    }

    bool isShowLatencyInMs() const
    {
        return isShowLatencyInMs_;
    }

private:
    double selectedOpacity_;
    double expandedProgress_;
    bool isShowLocationLoad_;
    bool isShowLatencyInMs_;
};

class IItemDelegate
{

public:
    virtual ~IItemDelegate() {};
    virtual void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index) const = 0;
    virtual QSize sizeHint(const QModelIndex &index) const = 0;
    virtual bool isForbiddenCursor(const QModelIndex &index) const = 0;

    // return -1 if point not in clickable area, otherwise, the ID of clickable area
    virtual int isInClickableArea(const QModelIndex &index, const QPoint &point) const = 0;
    // return -1 if point not in tooltip area, otherwise, the ID of tooltip area
    virtual int isInTooltipArea(const QModelIndex &index, const QPoint &point) const = 0;
};

} // namespace gui_location


#endif // GUI_LOCATIONS_IITEMDELEGATE_H

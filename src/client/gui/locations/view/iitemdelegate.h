#pragma once

#include <QWidget>
#include <QAbstractItemModel>
#include <QStyleOption>

namespace gui_locations {

class ItemStyleOption : public QStyleOption
{
public:
    explicit ItemStyleOption(QObject *object, const QRect &rc, double selectedOpacity, double expandedProgress, bool isShowLocationLoad, bool isShowCountryFlagForCity, int hoverClickableId = -1) :
        selectedOpacity_(selectedOpacity),
        expandedProgress_(expandedProgress),
        isShowLocationLoad_(isShowLocationLoad),
        isShowCountryFlagForCity_(isShowCountryFlagForCity),
        hoverClickableId_(hoverClickableId)
    {
        styleObject = object;
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

    bool isShowCountryFlagForCity() const
    {
        return isShowCountryFlagForCity_;
    }

    int hoverClickableId() const
    {
        return hoverClickableId_;
    }

private:
    double selectedOpacity_;
    double expandedProgress_;
    bool isShowLocationLoad_;
    bool isShowCountryFlagForCity_;
    int hoverClickableId_;
};

class IItemCacheData
{
public:
    virtual ~IItemCacheData() {}
};

class IItemDelegate
{
public:
    virtual ~IItemDelegate() {}
    virtual void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const = 0;
    virtual IItemCacheData *createCacheData(const QModelIndex &index) const = 0;
    virtual void updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const = 0;
    virtual bool isForbiddenCursor(const QModelIndex &index) const = 0;

    // return -1 if point not in clickable area, otherwise, the ID of clickable area
    virtual int isInClickableArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point) const = 0;
    // return -1 if point not in tooltip area, otherwise, the ID of tooltip area
    virtual int isInTooltipArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point, const IItemCacheData *cacheData) const = 0;

    virtual void tooltipEnterEvent(const ItemStyleOption &option, const QModelIndex &index, int tooltipId, const IItemCacheData *cacheData) const = 0;
    virtual void tooltipLeaveEvent(int tooltipId) const = 0;

};

} // namespace gui_locations



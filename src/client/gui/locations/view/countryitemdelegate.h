#pragma once

#include "iitemdelegate.h"

namespace gui_locations {

class CountryItemDelegate : public IItemDelegate
{
public:
    virtual ~CountryItemDelegate() {}
    void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const override;
    IItemCacheData *createCacheData(const QModelIndex &index) const override;
    void updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const override;
    bool isForbiddenCursor(const QModelIndex &index) const override;

    int isInClickableArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point) const override;
    int isInTooltipArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point, const IItemCacheData *cacheData) const override;

    void tooltipEnterEvent(const ItemStyleOption &option, const QModelIndex &index, int tooltipId, const IItemCacheData *cacheData) const override;
    void tooltipLeaveEvent(int tooltipId) const override;

private:
    const int kCountryItemMaxWidth = 218;

    QRect p2pRect(const QRect &itemRect) const;
    QRect captionRect(const QRect &itemRect, const IItemCacheData *cacheData) const;

};

} // namespace gui_locations

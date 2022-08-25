#pragma once

#include "iitemdelegate.h"

namespace gui_locations {

class CountryItemDelegate : public IItemDelegate
{
public:
    virtual ~CountryItemDelegate() {}
    void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const override;
    IItemCacheData *createCacheData(const QModelIndex &index) const override;
    bool isForbiddenCursor(const QModelIndex &index) const override;

    int isInClickableArea(const QModelIndex &index, const QPoint &point, const QRect &itemRect) const override;
    int isInTooltipArea(const QModelIndex &index, const QPoint &point) const override;
};

} // namespace gui_locations


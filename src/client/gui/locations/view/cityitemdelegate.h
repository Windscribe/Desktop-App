#pragma once

#include "iitemdelegate.h"

namespace gui_locations {

class CityItemDelegate : public IItemDelegate
{

public:
    virtual ~CityItemDelegate() {}
    void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const override;
    IItemCacheData *createCacheData(const QModelIndex &index) const override;
    void updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const override;
    bool isForbiddenCursor(const QModelIndex &index) const override;

    int isInClickableArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point) const override;
    int isInTooltipArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point, const IItemCacheData *cacheData) const override;

    void tooltipEnterEvent(const ItemStyleOption &option, const QModelIndex &index, int tooltipId, const IItemCacheData *cacheData) const override;
    void tooltipLeaveEvent(int tooltipId) const override;

private:
    const int kCityItemMaxWidth = 202;
    const int kCityCaptionMaxWidth = 102;

    QString pingIconNameString(int connectionSpeedIndex) const;
    QRect customConfigErrorRect(const QRect &itemRect) const;
    QRect captionRect(const QRect &itemRect, const IItemCacheData *cacheData) const;
    QRect nicknameRect(const QRect &itemRect, const IItemCacheData *cacheData) const;
};

} // namespace gui_locations

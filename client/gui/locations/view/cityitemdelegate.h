#pragma once

#include "iitemdelegate.h"

namespace gui_locations {

class CityItemDelegate : public IItemDelegate
{

public:
    virtual ~CityItemDelegate() {}
    void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index) const override;
    bool isForbiddenCursor(const QModelIndex &index) const override;

    int isInClickableArea(const QModelIndex &index, const QPoint &point, const QRect &itemRect) const override;
    int isInTooltipArea(const QModelIndex &index, const QPoint &point) const override;

private:
    const int CITY_CAPTION_MAX_WIDTH = 210;

    QString pingIconNameString(int connectionSpeedIndex) const;
};

} // namespace gui_locations


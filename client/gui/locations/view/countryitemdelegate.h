#ifndef GUI_LOCATIONS_COUNTRYITEMDELEGATE_H
#define GUI_LOCATIONS_COUNTRYITEMDELEGATE_H

#include "iitemdelegate.h"

namespace gui_location {

class CountryItemDelegate : public IItemDelegate
{
public:
    virtual ~CountryItemDelegate() {};
    void paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index) const override;
    QSize sizeHint(const QModelIndex &index) const override;
    bool isForbiddenCursor(const QModelIndex &index) const override;

    int isInClickableArea(const QModelIndex &index, const QPoint &point) const override;
    int isInTooltipArea(const QModelIndex &index, const QPoint &point) const override;
};

} // namespace gui_location


#endif // GUI_LOCATIONS_COUNTRYITEMDELEGATE_H

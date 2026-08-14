#pragma once

#include <QObject>
#include "tooltiptypes.h"
#include "itooltip.h"

class TooltipController : public QObject
{
    Q_OBJECT
public:
    static TooltipController &instance()
    {
        static TooltipController t;
        return t;
    }

    void hideAllTooltips();
    void showTooltipBasic(TooltipInfo info);
    void showTooltipDescriptive(TooltipInfo info);
    void hideTooltip(TooltipId type);

private:
    TooltipController();

    QMap<TooltipId, ITooltip*> tooltips_;

};

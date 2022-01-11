#ifndef TOOLTIPUTIL_H
#define TOOLTIPUTIL_H

#include <QString>


class TooltipUtil
{
public:
    static void getFirewallBlockedTooltipInfo(QString *title, QString *desc);
    static void getFirewallAlwaysOnTooltipInfo(QString *title, QString *desc);
};

#endif // TOOLTIPUTIL_H

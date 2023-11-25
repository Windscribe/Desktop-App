#pragma once

#include <QString>

class TooltipUtil
{
public:
    static void getFirewallBlockedTooltipInfo(QString *title, QString *desc);
    static void getFirewallAlwaysOnTooltipInfo(QString *title, QString *desc);
};

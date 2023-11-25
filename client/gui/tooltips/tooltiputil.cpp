#include "tooltiputil.h"
#include <QObject>

// static
void TooltipUtil::getFirewallAlwaysOnTooltipInfo(QString *title, QString *desc)
{
    if (title) *title = QObject::tr("Firewall Always On");
    if (desc)
        *desc = QObject::tr("Can't turn the firewall off because \"Always On\" mode is enabled");
}

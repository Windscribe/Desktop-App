#include "tooltiputil.h"
#include <QObject>

// static
void TooltipUtil::getFirewallBlockedTooltipInfo(QString *title, QString *desc)
{
    if (title) *title = QObject::tr("Firewall Disabled");
    if (desc) *desc = QObject::tr("Can't use the firewall when Split Tunneling is enabled");
}

#include "guisettingsfromver1.h"

#include <QSettings>
#include "Utils/logger.h"

ProtoTypes::GuiSettings GuiSettingsFromVer1::read()
{
    ProtoTypes::GuiSettings gs;
    QSettings settings("Windscribe", "Windscribe");

    if (settings.contains("launchOnStart"))
    {
        gs.set_is_launch_on_startup(settings.value("launchOnStart").toBool());
    }

    if (settings.contains("autoConnect"))
    {
        gs.set_is_auto_connect(settings.value("autoConnect").toBool());
    }

    #ifdef Q_OS_WIN
        if (settings.contains("minimizeToTray"))
        {
            gs.set_is_minimize_and_close_to_tray(settings.value("minimizeToTray").toBool());
        }
    #endif

    #ifdef Q_OS_MAC
        if (settings.contains("hideFromDock"))
        {
            gs.set_is_hide_from_dock(settings.value("hideFromDock").toBool());
        }
    #endif

    if (settings.contains("showNotifications"))
    {
        gs.set_is_show_notifications(settings.value("showNotifications").toBool());
    }

    if (settings.contains("showLatencyInMs"))
    {
        if (settings.value("showLatencyInMs").toBool())
        {
            gs.set_latency_display(ProtoTypes::LATENCY_DISPLAY_MS);
        }
        else
        {
            gs.set_latency_display(ProtoTypes::LATENCY_DISPLAY_BARS);
        }
    }

    if (settings.contains("orderLocationsType"))
    {
        int i = settings.value("orderLocationsType").toInt();
        if (i >= (int)0 && i <= (int)2)
        {
            gs.set_order_location((ProtoTypes::OrderLocationType)i);
        }
    }
    qCDebug(LOG_BASIC) << "Gui user settings:" << QString::fromStdString(gs.DebugString());
    return gs;
}

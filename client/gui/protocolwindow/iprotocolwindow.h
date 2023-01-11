#pragma once

#include <QGraphicsObject>
#include <QString>
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/resizablewindow.h"
#include "protocolwindow/protocolwindowmode.h"

class IProtocolWindow : public ResizableWindow
{
public:

    explicit IProtocolWindow(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
        : ResizableWindow(parent, preferences, preferencesHelper) {}
    virtual ~IProtocolWindow() {}

    virtual void setMode(ProtocolWindowMode mode) = 0;
    virtual void setProtocolStatus(const types::ProtocolStatus &status) = 0;
    virtual void setProtocolStatus(const QVector<types::ProtocolStatus> &status) = 0;
    virtual void resetProtocolStatus() = 0;

signals:
    virtual void protocolClicked(types::Protocol protocol, uint port) = 0;
    virtual void setAsPreferredProtocol(types::ConnectionSettings settings) = 0;
    virtual void sendDebugLog() = 0;
    virtual void stopConnection() = 0;
};

Q_DECLARE_INTERFACE(IProtocolWindow, "IProtocolWindow")
#pragma once

#include <QKeyEvent>
#include <QSet>
#include "../../common/types/protocolstatus.h"
#include "../commongraphics/textbutton.h"
#include "../backend/backend.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "commongraphics/imageitem.h"
#include "commongraphics/basepage.h"
#include "connectwindow/iconnectwindow.h"
#include "protocollistbutton.h"
#include "protocolwindow/iprotocolwindow.h"
#include "protocolwindow/protocollineitem.h"

namespace ProtocolWindow {

class ProtocolPromptItem: public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit ProtocolPromptItem(ScalableGraphicsObject *parent,
                                IConnectWindow *connectWindow,
                                Preferences *preferences,
                                PreferencesHelper *preferencesHelper,
                                ProtocolWindowMode mode);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) override;
    void updateScaling() override;

    ProtocolWindowMode mode();
    void setMode(ProtocolWindowMode mode);

    void clearCountdown();
    void setProtocolStatus(const types::ProtocolStatus &status);
    void setProtocolStatus(const QVector<types::ProtocolStatus> &status);

public slots:
    void resetProtocolStatus();

signals:
    void escape();
    void protocolClicked(types::Protocol protocol, uint port);
    void heightChanged(int height);
    void setAsPreferredProtocol(types::ConnectionSettings settings);
    void sendDebugLog();
    void stopConnection();

private slots:
    void onSetAsPreferred();
    void onSendDebugLog();
    void onContactSupport();
    void onProtocolClicked();
    void onTimerTimeout();
    void onCancelClicked();

private:
    static constexpr int kSpacerHeight = 16;
    static constexpr int kIndent = 36;

    const char *kChangeProtocolTitle = "Change Protocol";
    const char *kChangeProtocolDescription = "Quickly re-connect using a different protocol.";
    const char *kFailedProtocolTitle = "Connection Failure!";
    const char *kFailedProtocolDescription = "The protocol you’ve chosen has failed to connect. Windscribe will attempt to reconnect using the first protocol below.";
    const char *kSavePreferredProtocolTitle = "Set “%1” as preferred protocol?";
    const char *kSavePreferredProtocolDescription = "Windscribe will always use this protocol to connect on this network in the future to avoid any interruptions.";
    const char *kSendDebugLogTitle = "This network hates us";
    const char *kSendDebugLogDescription = "We couldn’t connect you on this network. Send us your debug log so we can figure out what happened.";
    const char *kDebugLogSentTitle = "Debug Sent!";
    const char *kDebugLogSentDescription = "Your debug log has been received. Please contact support if you want assistance with this issue.";

    const char *kSetAsPreferredButton = "Set as Preferred";
    const char *kSendDebugLogButton = "Send Debug Log";
    const char *kContactSupportButton = "Contact Support";
    const char *kCancelButton = "Cancel";

    QScopedPointer<ImageItem> icon_;

    QString title_;
    QString desc_;

    int titleHeight_;
    int descHeight_;

    ProtocolListButton *actionButton_;
    ProtocolListButton *cancelButton_;

    ProtocolWindowMode mode_;
    IConnectWindow *connectWindow_;
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    types::Protocol lastProtocol_;
    uint lastPort_;
    QTimer countdownTimer_;
    bool resetProtocolStatus_;

    QVector<types::ProtocolStatus> statuses_;
    QVector<ProtocolLineItem *> items_;

    void addCancelButton();
    void setIcon(const QString &path);
    QString getProtocolDescription(const types::Protocol &protocol);
    void updateItemOffset();
    void doResetProtocolStatus();
    types::Protocol connectedProtocol();
};

} // namespace ProtocolWindow
#pragma once

#include <QKeyEvent>
#include <QSet>
#include "../../common/types/protocolstatus.h"
#include "../backend/backend.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "commongraphics/imageitem.h"
#include "commongraphics/basepage.h"
#include "commongraphics/listbutton.h"
#include "connectwindow/iconnectwindow.h"
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
    void clearItems() override;

public slots:
    void resetProtocolStatus();

signals:
    void escape();
    void protocolClicked(types::Protocol protocol, uint port);
    void heightChanged(int height);
    void stopConnection();

private slots:
    void onProtocolClicked();
    void onTimerTimeout();
    void onCancelClicked();
    void onLanguageChanged();

private:
    static constexpr int kSpacerHeight = 16;
    static constexpr int kIndent = 36;

    const char *kChangeProtocolTitle = QT_TR_NOOP("Change Protocol");
    const char *kChangeProtocolDescription = QT_TR_NOOP("Quickly re-connect using a different protocol.");
    const char *kFailedProtocolTitle = QT_TR_NOOP("Connection Failure!");
    const char *kFailedProtocolDescription = QT_TR_NOOP("The protocol you’ve chosen has failed to connect. Windscribe will attempt to reconnect using the first protocol below.");
    const char *kCancelButton = QT_TR_NOOP("Cancel");

    QScopedPointer<ImageItem> icon_;

    QString title_;
    QString desc_;

    int titleHeight_;
    int descHeight_;

    CommonGraphics::ListButton *cancelButton_;

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
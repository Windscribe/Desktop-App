#pragma once

#include <QFont>
#include <QTimer>
#include <QVariantAnimation>
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/iconbutton.h"
#include "backend/preferences/preferences.h"
#include "graphicresources/fontdescr.h"
#include "graphicresources/independentpixmap.h"
#include "utils/textshadow.h"
#include "utils/imagewithshadow.h"
#include "connectionbadgedots.h"
#include "badgepixmap.h"
#include "types/connectstate.h"
#include "types/protocol.h"
#include "types/protocolstatus.h"

namespace ConnectWindow {

class ConnectStateProtocolPort : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit ConnectStateProtocolPort(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void onConnectStateChanged(const types::ConnectState &newConnectState, const types::ConnectState &prevConnectState);

    void setHoverable(bool hoverable);
    void setInternetConnectivity(bool connectivity);
    void setProtocolPort(const types::Protocol &protocol, const uint port);
    void setTestTunnelResult(bool success);
    bool receivedTunnelTestResult() const;
    types::ProtocolStatus getProtocolStatus();
    bool isProtocolButtonVisible() const;
    void setProtocolButtonVisible(bool visible);
    void setIsPreferredProtocol(bool on);

    void updateScaling() override;

signals:
    void protocolsClick();

public slots:
    void antiCensorshipChanged(bool enabled);

private slots:
    void onProtocolTestTunnelTimerTick();
    void onProtocolOpacityAnimationChanged(const QVariant &value);
    void onProtocolArrowAnimationChanged(const QVariant &value);
    void onHoverEnter();
    void onHoverLeave();

private:
    ConnectionBadgeDots * connectionBadgeDots_;

    FontDescr fontDescr_;
    types::ConnectState connectState_;
    types::ConnectState prevConnectState_;
    bool connectivity_;
    types::Protocol protocol_;
    uint port_;
    QColor textColor_;
    double textOpacity_;
    bool receivedTunnelTestResult_;

    //QString badgeIconFg_;

    QSharedPointer<IndependentPixmap> badgeFgImage_;

    QColor badgeBgColor_;
    BadgePixmap badgePixmap_;

    IconButton *preferredProtocolBadge_;
    bool isPreferredProtocol_;

    IconButton *protocolArrow_;
    int arrowShift_;

    IconButton *antiCensorshipBadge_;
    bool isAntiCensorshipEnabled_ = false;

    int width_;
    int height_;
    void recalcSize();
    void updateStateDisplay(const types::ConnectState &newConnectState);

    static constexpr int kSpacerWidth = 8;

    QTimer protocolTestTunnelTimer_;
    QVariantAnimation protocolOpacityAnimation_;
    QVariantAnimation protocolArrowAnimation_;
};

} //namespace ConnectWindow

#ifndef CONNECTSTATETEXT_H
#define CONNECTSTATETEXT_H

#include <QFont>
#include <QTimer>
#include <QVariantAnimation>
#include "../Backend/Types/types.h"
#include "CommonGraphics/scalablegraphicsobject.h"
#include "../Backend/Preferences/preferences.h"
#include "IPC/generated_proto/types.pb.h"
#include "GraphicResources/fontdescr.h"

namespace ConnectWindow {

class ConnectStateProtocolPort : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ConnectStateProtocolPort(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void onConnectStateChanged(ProtoTypes::ConnectState newConnectState, ProtoTypes::ConnectState prevConnectState);

    void setHoverable(bool hoverable);
    void setInternetConnectivity(bool connectivity);
    void setProtocolPort(const ProtoTypes::Protocol &protocol, const uint port);
    void setTestTunnelResult(bool success);

    void updateScaling() override;

signals:
    void hoverEnter();
    void hoverLeave();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onProtocolTestTunnelTimerTick();
    void onProtocolOpacityAnimationChanged(const QVariant &value);

private:

    FontDescr fontDescr_;
    ProtoTypes::ConnectState connectState_;
    ProtoTypes::ConnectState prevConnectState_;
    bool hoverable_;
    bool connectivity_;
    ProtoTypes::Protocol protocol_;
    uint port_;

    QColor textColor_;
    double textOpacity_;
    QString badgeIconBg_;
    QString badgeIconFg_;

    int width_;
    int height_;
    void recalcSize();
    void updateStateDisplay(ProtoTypes::ConnectState newConnectState);

    const int badgeProtocolPadding = 10;
    const int protocolSeparatorPadding = 7;
    const int separatorPortPadding = 8;

    bool receivedTunnelTestResult_;
    QTimer protocolTestTunnelTimer_;
    QVariantAnimation protocolOpacityAnimation_;

};

} //namespace ConnectWindow

#endif // CONNECTSTATETEXT_H

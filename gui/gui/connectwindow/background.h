#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QTimer>
#include "../backend/types/types.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace ConnectWindow {

class Background : public ScalableGraphicsObject
{
    Q_PROPERTY(qreal opacityConnecting READ opacityConnecting WRITE setOpacityConnecting)
    Q_PROPERTY(qreal opacityConnected READ opacityConnected WRITE setOpacityConnected)
    Q_PROPERTY(qreal opacityDisconnected READ opacityDisconnected WRITE setOpacityDisconnected)
    Q_OBJECT
public:
    explicit Background(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void onConnectStateChanged(ProtoTypes::ConnectStateType newConnectState, ProtoTypes::ConnectStateType prevConnectState);
    void onLocationSelected(const QString &countryCode);

    void setDarkMode(bool dark);

    qreal opacityConnecting();
    void setOpacityConnecting(qreal v);

    qreal opacityConnected();
    void setOpacityConnected(qreal v);

    qreal opacityDisconnected();
    void setOpacityDisconnected(qreal v);

    QPixmap getShadowPixmap();

private slots:
     void onFlagOpacityChanged(const QVariant &value);

private:
    static constexpr int ANIMATION_DURATION = 600;
    static constexpr int WIDTH = 332;
    static constexpr int HEIGHT = 316;
    qreal opacityConnecting_;
    qreal opacityConnected_;
    qreal opacityDisconnected_;
    QPropertyAnimation opacityConnectingAnimation_;
    QPropertyAnimation opacityConnectedAnimation_;
    QPropertyAnimation opacityDisconnectedAnimation_;

    QString prevCountryCode_;
    QString countryCode_;
    QVariantAnimation opacityFlagAnimation_;
    qreal opacityCurFlag_;
    qreal opacityPrevFlag_;

    QString topFrameBG_         ;
    QString flagGradient_       ;
    QString connectingGradient_ ;
    QString connectedGradient_  ;
    QString headerDisconnected_ ;
    QString headerConnected_    ;
    QString headerConnecting_   ;
    QString bottomFrameBG_      ;

    QString midRightVertDivider_;
    QString bottomLeftHorizDivider_;
};

} //namespace ConnectWindow

#endif // BACKGROUND_H

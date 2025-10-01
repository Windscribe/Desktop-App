#pragma once

#include <QGraphicsObject>
#include <QPropertyAnimation>
#include "commongraphics/clickablegraphicsobject.h"
#include "commongraphics/imageitem.h"
#include "types/connectstate.h"

namespace ConnectWindow {

class ConnectButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit ConnectButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QPainterPath shape() const override;

    void onConnectStateChanged(CONNECT_STATE newConnectState, CONNECT_STATE prevConnectState);
    void setInternetConnectivity(bool online);
    void setSplitRouting(bool on);

    void updateScaling() override;

private slots:
    void onConnectingRingOpacityAnimationFinished();
    void onNoInternetRingOpacityAnimationFinished();
    void onButtonRotationAnimationValueChanged(const QVariant &value);
    void onConnectingRingRotationAnimationValueChanged(const QVariant &value);
    void onConnectingRingOpacityAnimationValueChanged(const QVariant &value);

private:
    CONNECT_STATE connectStateType_;

    ImageItem *svgItemButton_;
    ImageItem *svgItemConnectedRing_;
    ImageItem *svgItemConnectedSplitRoutingRing_;
    ImageItem *svgItemConnectingRing_;
    ImageItem *svgItemConnectingRingShadow_;
    ImageItem *svgItemConnectingNoInternetRing_;
    ImageItem *svgItemButtonShadow_;
    bool online_;
    bool splitRouting_;

    QPropertyAnimation buttonRotationAnimation_;
    QPropertyAnimation connectingRingRotationAnimation_;
    QPropertyAnimation connectingRingOpacityAnimation_;
    QPropertyAnimation noInternetRingRotationAnimation_;
    QPropertyAnimation noInternetRingOpacityAnimation_;
    QPropertyAnimation connectedRingOpacityAnimation_;
    QPropertyAnimation connectedSplitRoutingRingOpacityAnimation_;

    void fuzzyHideConnectedRing();
    void fuzzyHideConnectingRing();
    void fuzzyHideNoInternetRing();

    void updatePositions();
    void startConnectedRingAnimations(QPropertyAnimation::Direction dir);
    void startConnectingRingAnimations(QPropertyAnimation::Direction dir);
    void startNoInternetRingAnimations(QPropertyAnimation::Direction dir);
};

} //namespace ConnectWindow

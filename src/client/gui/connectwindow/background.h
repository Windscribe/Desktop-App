#pragma once

#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QTimer>
#include "commongraphics/scalablegraphicsobject.h"
#include "backgroundimage/backgroundimage.h"
#include "utils/imagewithshadow.h"

namespace ConnectWindow {

class Background : public ScalableGraphicsObject
{
    Q_PROPERTY(qreal opacityConnecting READ opacityConnecting WRITE setOpacityConnecting)
    Q_PROPERTY(qreal opacityDisconnected READ opacityDisconnected WRITE setOpacityDisconnected)
    Q_OBJECT
public:
    explicit Background(ScalableGraphicsObject *parent, Preferences *preferences);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void onConnectStateChanged(CONNECT_STATE newConnectState, CONNECT_STATE prevConnectState);
    void onLocationSelected(const QString &countryCode);
    void setDarkMode(bool dark);
    QPixmap getShadowPixmap();

    void updateScaling() override;

private slots:
    void doUpdate();

private:
    static constexpr int ANIMATION_DURATION = 600;
    static constexpr int WIDTH = 350;
    static constexpr int HEIGHT = 271;

    Preferences *preferences_;

    qreal opacityConnecting_;
    qreal opacityDisconnected_;
    QPropertyAnimation opacityConnectingAnimation_;
    QPropertyAnimation opacityDisconnectedAnimation_;

    BackgroundImage backgroundImage_;

    QString mainBG_;
    QString borderInner_;
    QString bottomFrameBG_;

    qreal opacityConnecting();
    void setOpacityConnecting(qreal v);

    qreal opacityDisconnected();
    void setOpacityDisconnected(qreal v);
};

} //namespace ConnectWindow

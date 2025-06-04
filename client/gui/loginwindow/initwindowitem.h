#pragma once

#include <QVariantAnimation>
#include <QTimer>
#include "commongraphics/iconbutton.h"

namespace LoginWindow {

class InitWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit InitWindowItem(QGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void resetState();
    void startSlideAnimation();
    void startWaitingAnimation();

    void setAdditionalMessage(const QString &msg, bool useSmallFont);
    void setCropHeight(int height);
    void setHeight(int height);
    void setCloseButtonVisible(bool visible);

signals:
    void abortClicked();

private slots:
    void onSpinnerRotationChanged(const QVariant &value);
    void onSpinnerRotationFinished();

    void onLogoPosChanged(const QVariant &value);

private:
    static constexpr int LOGO_POS_TOP = 87;
    static constexpr int LOGO_POS_CENTER = 120;
    static constexpr int ABORT_POS_Y = 225;
    static constexpr int SPINNER_SPEED = 500;

    static constexpr int MSG_SMALL_FONT = 12;
    static constexpr int MSG_LARGE_FONT = 16;

    void updatePositions();

    IconButton *closeButton_;

    int curLogoPosY_;
    int curSpinnerRotation_;
    double curSpinnerOpacity_;

    bool waitingAnimationActive_;

    int cropHeight_;
    int height_;

    int logoDist_;

    QString msg_;
    bool isgMsgSmallFont_;

    QVariantAnimation logoPosAnimation_;
    QVariantAnimation spinnerRotationAnimation_;

    QVariantAnimation abortOpacityAnimation_;
};

}

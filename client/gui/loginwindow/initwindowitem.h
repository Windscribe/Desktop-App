#ifndef INITWINDOWITEM_H
#define INITWINDOWITEM_H

#include <QVariantAnimation>
#include <QTimer>
#include "iinitwindow.h"
#include "commongraphics/bubblebuttondark.h"
#include "commongraphics/iconbutton.h"

namespace LoginWindow {

class InitWindowItem : public ScalableGraphicsObject, public IInitWindow
{
    Q_OBJECT
    Q_INTERFACES(IInitWindow)
public:
    explicit InitWindowItem(QGraphicsObject *parent = nullptr);

    QGraphicsObject *getGraphicsObject() override { return this; }
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void resetState() override ;
    void startSlideAnimation() override;
    void startWaitingAnimation() override;

    void setClickable(bool clickable) override;
    void setAdditionalMessage(const QString &msg, bool useSmallFont) override;
    void setCropHeight(int height) override;
    void setHeight(int height) override;
    void setCloseButtonVisible(bool visible) override;

    void updateScaling() override;

signals:
    void abortClicked() override;

private slots:
    void onSpinnerRotationChanged(const QVariant &value);
    void onSpinnerRotationFinished();

    void onLogoPosChanged(const QVariant &value);

    void onShowAbortTimerTimeout();

private:
    static constexpr int LOGO_POS_TOP = 87;
    static constexpr int LOGO_POS_CENTER = 135;
    static constexpr int ABORT_POS_Y = 225;
    static constexpr int SPINNER_SPEED = 500;

    static constexpr int MSG_SMALL_FONT = 12;
    static constexpr int MSG_LARGE_FONT = 16;

    void updatePositions();

    IconButton *closeButton_;

    int curLogoPosY_;
    int curSpinnerRotation_;
    double curSpinnerOpacity_;
    double curAbortOpacity_;

    bool waitingAnimationActive_;

    int cropHeight_;
    int height_;

    int logoDist_;

    QString msg_;
    bool isgMsgSmallFont_;

    QVariantAnimation logoPosAnimation_;
    QVariantAnimation spinnerRotationAnimation_;

    QVariantAnimation abortOpacityAnimation_;
    //CommonGraphics::BubbleButtonDark *abortButton_;

    //QTimer showAbortTimer_;

};

}

#endif // INITWINDOWITEM_H

#pragma once

#include <QEvent>
#include <QGraphicsObject>
#include "backend/backend.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "textlinkitem.h"

namespace EmergencyConnectWindow {

class EmergencyConnectWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit EmergencyConnectWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;
    void updateScaling() override;

    void setState(types::ConnectState state);
    void setClickable(bool isClickable);

signals:
    void escapeClick();
    void connectClick();
    void disconnectClick();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEscClicked();

    void onConnectClicked();
    void onDisconnectClicked();

    void onTitleOpacityChange(const QVariant &value);
    void onDescriptionTransition(const QVariant &value);
    void onIconOpacityChange(const QVariant &value);

    void onSpinnerTransition(const QVariant &value );
    void onSubDescriptionTextTransition(const QVariant &value);
    void onSpinnerRotation(const QVariant &value);

    void onLanguageChanged();

private:
    types::ConnectState state_;
    bool errorConnecting_;

    void startSpinnerAnimation();
    void stopSpinnerAnimation();

    void transitionToConnecting();
    void transitionToConnect();

    void transitionToDisconnecting();
    void transitionToDisconnect();

    void updatePositions();

    QString curSubDescription_;

    CommonGraphics::BubbleButton *connectButton_;
    CommonGraphics::BubbleButton *disconnectButton_;

    CommonGraphics::EscapeButton *escButton_;
    EmergencyConnectWindow::TextLinkItem *accessText_;

    double curTitleOpacity_;
    double curIconOpacity_;

    QVariantAnimation titleOpacityAnimation_;
    QVariantAnimation descriptionOpacityAnimation_;
    QVariantAnimation iconOpacityAnimation_;

    QVariantAnimation spinnerOpacityAnimation_;
    QVariantAnimation subDescriptionTextOpacityAnimation_;

    bool spinnerRotationAnimationActive_;

    double curSpinnerPageOpacity_;
    double curDescriptionOpacity_;
    double curSubDescriptionTextOpacity_;

    QVariantAnimation spinnerRotationAnimation_;
    int curSpinnerRotation_;
    int curTargetRotation_;
    int lastSpinnerRotationStart_;

    // constants:
    static constexpr int ICON_POS_Y = 48;

    static constexpr int TITLE_POS_Y = 104;
    static constexpr int DESCRIPTION_POS_Y = 140;
    static constexpr int DESCRIPTION_WIDTH_MIN = 218;

    static constexpr int LINK_TEXT_POS_Y = 307;

    static constexpr int CONNECTING_TEXT_POS_Y = 222;
    static constexpr int CONNECTING_TEXT_HEIGHT = 20;

    static constexpr int SPINNER_POS_Y = 164;

    static constexpr int CONNECT_BUTTON_POS_Y = 249;
    static constexpr int DISCONNECT_BUTTON_POS_Y = 249;

    static constexpr int TARGET_ROTATION = 360;
};

}

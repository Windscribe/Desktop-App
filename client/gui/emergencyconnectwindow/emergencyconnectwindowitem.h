#ifndef EMERGENCYCONNECTWINDOWITEM_H
#define EMERGENCYCONNECTWINDOWITEM_H

#include <QGraphicsObject>
#include <QEvent>
#include "../backend/backend.h"
#include "emergencyconnectwindow/iemergencyconnectwindow.h"
#include "commongraphics/bubblebuttonbright.h"
#include "commongraphics/bubblebuttondark.h"
#include "commongraphics/iconbutton.h"
#include "textlinkbutton.h"
#include "preferenceswindow/escapebutton.h"

namespace EmergencyConnectWindow {


class EmergencyConnectWindowItem : public ScalableGraphicsObject, public IEmergencyConnectWindow
{
    Q_OBJECT
    Q_INTERFACES(IEmergencyConnectWindow)
public:
    explicit EmergencyConnectWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    QGraphicsObject *getGraphicsObject() override { return this; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setState(ProtoTypes::ConnectState state) override;
    void setClickable(bool isClickable) override;

    void updateScaling() override;

signals:
    void minimizeClick() override;
    void closeClick() override;
    void escapeClick() override;
    void connectClick() override;
    void windscribeLinkClick() override;
    void disconnectClick() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEscClicked();

    void onConnectClicked();
    void onDisconnectClicked();

    void onTitleOpacityChange(const QVariant &value);
    void onDescriptionTransition(const QVariant &value);
    void onIconOpacityChange(const QVariant &value);
    void onMinimizeCloseOpacityChange(const QVariant &value);

    void onSpinnerTransition(const QVariant &value );
    void onSubDescriptionTextTransition(const QVariant &value);
    void onSpinnerRotation(const QVariant &value);

    void onTextLinkWidthChanged();

    void onLanguageChanged();
    void onDockedModeChanged(bool bIsDockedToTray);

private:
    ProtoTypes::ConnectState state_;
    bool errorConnecting_;

    void startSpinnerAnimation();
    void stopSpinnerAnimation();

    void transitionToConnecting();
    void transitionToConnect();

    void transitionToDisconnecting();
    void transitionToDisconnect();

    void updatePositions();

    QString curSubDescription_;

    CommonGraphics::BubbleButtonDark *connectButton_;
    CommonGraphics::BubbleButtonBright *disconnectButton_;

    PreferencesWindow::EscapeButton *escButton_;
    TextLinkButton *textLinkButton_;

    double curTitleOpacity_;
    double curIconOpacity_;
    double curMinimizeCloseOpacity_;

    QVariantAnimation titleOpacityAnimation_;
    QVariantAnimation descriptionOpacityAnimation_;
    QVariantAnimation iconOpacityAnimation_;
    QVariantAnimation minimizeCloseOpacityAnimation_;

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

    IconButton *closeButton_;
    IconButton *minimizeButton_;

    // constants:
    static constexpr int ICON_POS_Y = 48;

    static constexpr int TITLE_POS_Y = 104;
    static constexpr int DESCRIPTION_POS_Y = 140;
    static constexpr int DESCRIPTION_WIDTH_MIN = 218;

    static constexpr int LINK_TEXT_POS_Y = 281;

    static constexpr int CONNECTING_TEXT_POS_Y = 222;
    static constexpr int CONNECTING_TEXT_HEIGHT = 20;

    static constexpr int SPINNER_POS_Y = 164;

    static constexpr int CONNECT_BUTTON_POS_Y = 233;
    static constexpr int DISCONNECT_BUTTON_POS_Y = 233;

    static constexpr int TARGET_ROTATION = 360;

    const QString CONNECTING_STRING = QT_TR_NOOP("Connecting...");

};

}

#endif // EMERGENCYCONNECTWINDOWITEM_H

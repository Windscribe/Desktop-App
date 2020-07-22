#ifndef EMERGENCYCONNECTWINDOWITEM_H
#define EMERGENCYCONNECTWINDOWITEM_H

#include <QGraphicsObject>
#include <QEvent>
#include <EmergencyConnectWindow/iemergencyconnectwindow.h>
#include "CommonGraphics/bubblebuttonbright.h"
#include "CommonGraphics/bubblebuttondark.h"
#include "CommonGraphics/iconbutton.h"
#include "textlinkbutton.h"
#include "PreferencesWindow/escapebutton.h"

namespace EmergencyConnectWindow {


class EmergencyConnectWindowItem : public ScalableGraphicsObject, public IEmergencyConnectWindow
{
    Q_OBJECT
    Q_INTERFACES(IEmergencyConnectWindow)
public:
    explicit EmergencyConnectWindowItem(QGraphicsObject *parent = nullptr);

    virtual QGraphicsObject *getGraphicsObject() { return this; }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void setState(ProtoTypes::ConnectState state);
    virtual void setClickable(bool isClickable);

    virtual void updateScaling();

signals:
    void minimizeClick();
    void closeClick();

    void escapeClick();

    void connectClick();
    void windscribeLinkClick();

    void disconnectClick();

protected:
    void keyPressEvent(QKeyEvent *event);

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
    const int ICON_POS_Y = 48;

    const int TITLE_POS_Y = 104;
    const int DESCRIPTION_POS_Y = 140;
    const int DESCRIPTION_WIDTH_MIN = 218;

    const int LINK_TEXT_POS_Y = 281;

    const int CONNECTING_TEXT_POS_Y = 222;
    const int CONNECTING_TEXT_HEIGHT = 20;

    const int SPINNER_POS_Y = 164;

    const int CONNECT_BUTTON_POS_Y = 218;
    const int DISCONNECT_BUTTON_POS_Y = 218;

    const int TARGET_ROTATION = 360;

    const QString CONNECTING_STRING = QT_TR_NOOP("Connecting...");

};

}

#endif // EMERGENCYCONNECTWINDOWITEM_H

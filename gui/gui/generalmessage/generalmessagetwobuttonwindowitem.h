#ifndef GENERALMESSAGETWOBUTTONWINDOWITEM_H
#define GENERALMESSAGETWOBUTTONWINDOWITEM_H

#include <QGraphicsObject>
#include "igeneralmessagetwobuttonwindow.h"
#include "commongraphics/bubblebuttondark.h"

namespace GeneralMessage {

class GeneralMessageTwoButtonWindowItem : public ScalableGraphicsObject, public IGeneralMessageTwoButtonWindow
{
    Q_OBJECT
    Q_INTERFACES(IGeneralMessageTwoButtonWindow)
public:
    explicit GeneralMessageTwoButtonWindowItem(const QString title, const QString icon,
                                               const QString acceptText, const QString rejectText,
                                               QGraphicsObject *parent = nullptr);

    virtual QGraphicsObject *getGraphicsObject() override { return this; }
    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void setTitle(const QString title) override ;
    void setIcon(const QString icon)   override ;

    void setAcceptText(const QString text) override;
    void setRejectText(const QString text) override;

    void setBackgroundShapedToConnectWindow(bool shapedToConnectWindow) override;
    void setShutdownAnimationMode(bool isShutdownAnimationMode) override;

    void clearSelection() override;

    void updateScaling() override;

    void unselectCurrentButton(CommonGraphics::BubbleButtonDark *button);

signals:
    void acceptClick() override;
    void rejectClick() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private slots:
    void onHoverAccept();
    void onHoverLeaveAccept();
    void onHoverReject();
    void onHoverLeaveReject();

    void onSpinnerRotationChanged(const QVariant &value);
    void onSpinnerRotationFinished();

private:
    QString title_;
    QString titleShuttingDown_;
    QString icon_;

    const int RECT_ICON_POS_Y = 48;
    const int RECT_TITLE_POS_Y = 120;
    const int RECT_ACCEPT_BUTTON_POS_Y = 172;
    const int RECT_REJECT_BUTTON_POS_Y = 224;

    const int SHAPED_ICON_POS_Y = 58;
    const int SHAPED_TITLE_POS_Y = 120;
    const int SHAPED_ACCEPT_BUTTON_POS_Y = 167;
    const int SHAPED_REJECT_BUTTON_POS_Y = 219;

    int titlePosY_;
    int iconPosY_;

    bool shapedToConnectWindow_;
    QString backgroundIcon_;

    const int LOGO_POS_TOP = 25;
    const int LOGO_POS_CENTER = 100;
    const int ABORT_POS_Y = 225;
    const int SPINNER_SPEED = 500;

    bool isShutdownAnimationMode_;
    int curLogoPosY_;
    int curSpinnerRotation_;
    QVariantAnimation spinnerRotationAnimation_;

    CommonGraphics::BubbleButtonDark *acceptButton_;
    CommonGraphics::BubbleButtonDark *rejectButton_;

    enum Selection { NONE, ACCEPT, REJECT };
    Selection selection_;
    void changeSelection(Selection selection);
};

} // namespace GeneralMessage

#endif // GENERALMESSAGETWOBUTTONWINDOWITEM_H

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

    QGraphicsObject *getGraphicsObject() override { return this; }
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setTitle(const QString title) override ;
    void setIcon(const QString icon)   override ;

    void setAcceptText(const QString text) override;
    void setRejectText(const QString text) override;

    void setBackgroundShapedToConnectWindow(bool shapedToConnectWindow) override;
    void setHeight(int height) override;
    void setShutdownAnimationMode(bool isShutdownAnimationMode) override;

    void clearSelection() override;

    void updateScaling() override;

    void unselectCurrentButton(CommonGraphics::BubbleButtonDark *button);

    void changeSelection(Selection selection) override;

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
    void updatePositions();

    QString title_;
    QString icon_;

    static constexpr int SHAPED_ICON_POS_Y = 58;
    static constexpr int SHAPED_TITLE_POS_Y = 120;
    static constexpr int SHAPED_ACCEPT_BUTTON_POS_Y = 167;
    static constexpr int SHAPED_REJECT_BUTTON_POS_Y = 219;

    int height_;
    int titlePosY_;
    int iconPosY_;

    bool shapedToConnectWindow_;
    QString backgroundIcon_;

    static constexpr int LOGO_POS_TOP = 25;
    static constexpr int LOGO_POS_CENTER = 100;
    static constexpr int ABORT_POS_Y = 225;
    static constexpr int SPINNER_SPEED = 500;

    bool isShutdownAnimationMode_;
    int curLogoPosY_;
    int curSpinnerRotation_;
    QVariantAnimation spinnerRotationAnimation_;

    CommonGraphics::BubbleButtonDark *acceptButton_;
    CommonGraphics::BubbleButtonDark *rejectButton_;

    Selection selection_;
};

} // namespace GeneralMessage

#endif // GENERALMESSAGETWOBUTTONWINDOWITEM_H

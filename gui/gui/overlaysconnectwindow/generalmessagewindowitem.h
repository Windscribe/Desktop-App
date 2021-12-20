#ifndef GENERALMESSAGEWINDOWITEM_H
#define GENERALMESSAGEWINDOWITEM_H

#include <QGraphicsObject>
#include "igeneralmessagewindow.h"
#include "commongraphics/bubblebuttondark.h"

namespace GeneralMessage {

class GeneralMessageWindowItem : public ScalableGraphicsObject, public IGeneralMessageWindow
{
    Q_OBJECT
    Q_INTERFACES(IGeneralMessageWindow)
public:
    explicit GeneralMessageWindowItem(bool errorMode, QGraphicsObject *parent = nullptr);

    QGraphicsObject *getGraphicsObject() override { return this; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setTitle(QString title) override;
    void setDescription(QString description) override; // UI will currently only support <= 4 lines of description and still look good
    void setErrorMode(bool error) override;

    void updateScaling() override;

signals:
    void acceptClick() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onAcceptClick();

private:
    CommonGraphics::BubbleButtonDark *acceptButton_;

    QString curTitleText_;
    QString curDescriptionText_;

    double curBackgroundOpacity_;
    double curTitleOpacity_;
    double curDescriptionOpacity_;

    QColor titleColor_;

    // constants:
    static constexpr int TITLE_POS_Y = 69;

    static constexpr int DESCRIPTION_WIDTH_MIN = 230;
    static constexpr int DESCRIPTION_POS_Y = 145;

    static constexpr int ACCEPT_BUTTON_POS_Y = 200;

    QString background_;

};

}

#endif // GENERALMESSAGEWINDOWITEM_H

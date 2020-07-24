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

    virtual QGraphicsObject *getGraphicsObject() { return this; }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setTitle(QString title);
    void setDescription(QString description); // UI will currently only support <= 4 lines of description and still look good
    void setErrorMode(bool error);

signals:
    void acceptClick();

protected:
    void keyPressEvent(QKeyEvent *event);

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
    const int TITLE_POS_Y = 69;

    const int DESCRIPTION_WIDTH_MIN = 230;
    const int DESCRIPTION_POS_Y = 145;

    const int ACCEPT_BUTTON_POS_Y = 200;

    QString background_;

};

}

#endif // GENERALMESSAGEWINDOWITEM_H

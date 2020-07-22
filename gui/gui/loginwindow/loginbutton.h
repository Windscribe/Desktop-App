#ifndef LOGINBUTTON_H
#define LOGINBUTTON_H

#include "CommonGraphics/clickablegraphicsobject.h"


namespace LoginWindow {

class LoginButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:

    explicit LoginButton(ScalableGraphicsObject * parent = nullptr);
    virtual QRectF boundingRect() const;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void setClickable(bool clickable);
    void setError(bool error);

private:
    QString imagePath_;

    int width_;
    int height_;
};

}
#endif // LOGINBUTTON_H

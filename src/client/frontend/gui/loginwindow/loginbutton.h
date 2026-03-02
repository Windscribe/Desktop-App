#pragma once

#include "commongraphics/clickablegraphicsobject.h"

namespace LoginWindow {

class LoginButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:

    explicit LoginButton(ScalableGraphicsObject * parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setError(bool error);
    void setEnabled(bool bEnabled);

private:
    QString imagePath_;

    int width_;
    int height_;
    bool isEnabled_;
};

}

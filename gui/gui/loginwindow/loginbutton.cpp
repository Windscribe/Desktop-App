#include "loginbutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

LoginButton::LoginButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject (parent),
    imagePath_("login/DISABLED_LOGIN_ICON"), width_(32), height_(32)
{
}

QRectF LoginButton::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_*G_SCALE);
}

void LoginButton::setError(bool error)
{
    if (error)
    {
        imagePath_ = "login/ERROR_LOGIN_ICON";
    }
    else if (clickable_)
    {
        imagePath_ = "login/ENABLED_LOGIN_ICON";
    }
    else
    {
        imagePath_ = "login/DISABLED_LOGIN_ICON";
    }

    update();
}

void LoginButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();
    painter->setOpacity(initOpacity);

    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    pixmap->draw(0, 0, painter);
}

void LoginButton::setClickable(bool clickable)
{
    ClickableGraphicsObject::setClickable(clickable);

    if (clickable)
    {
        imagePath_ = "login/ENABLED_LOGIN_ICON";
    }
    else
    {
        imagePath_ = "login/DISABLED_LOGIN_ICON";
    }

    update();
}

}

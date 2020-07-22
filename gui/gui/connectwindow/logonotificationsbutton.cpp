#include "logonotificationsbutton.h"

#include <QPainter>
#include "GraphicResources/fontmanager.h"
#include "GraphicResources/imageresourcessvg.h"
#include "CommonGraphics/commongraphics.h"
#include "CommonGraphics/clickablegraphicsobject.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {

LogoNotificationsButton::LogoNotificationsButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent)
    , curBackgroundOpacity_(OPACITY_ALL_READ)
    , numNotifications_("0")
    , unread_(0)
{
    //font_ = *FontManager::instance().getFont(10, true);
    //numberWidth_ = CommonGraphics::textWidth(numNotifications_, font_);

    connect(this, SIGNAL(hoverEnter()), SLOT(onHoverEnter()));
    connect(this, SIGNAL(hoverLeave()), SLOT(onHoverLeave()));

    setCountState(0, 0);
}

QRectF LogoNotificationsButton::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, HEIGHT*G_SCALE);
}

void LogoNotificationsButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();
    painter->setRenderHint(QPainter::Antialiasing);

    // LOGO
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("LOGO");
    p->draw(0, 10*G_SCALE, painter);

    const int notificationXOffset = 120*G_SCALE;
    QFont *font = FontManager::instance().getFont(10, true);

    if (unread_ > 0 || hovered_)
    {
        // notification background
        painter->setFont(*font);
        painter->setOpacity(curBackgroundOpacity_ * initOpacity);
        painter->setBrush(backgroundFillColor_);
        painter->setPen(backgroundOutlineColor_);
        int numberWidth = CommonGraphics::textWidth(numNotifications_, *font);
        painter->drawRoundedRect(QRect(notificationXOffset, 0, numberWidth + MARGIN*2*G_SCALE, CommonGraphics::textHeight(*font) + 2*G_SCALE)
                                 , 3*G_SCALE, 3*G_SCALE);

        // notification number
        painter->setOpacity(initOpacity);
        painter->setPen(numberColor_);
        if (unread_ > 0)
        {
            painter->drawText(notificationXOffset + MARGIN*G_SCALE, CommonGraphics::textHeight(*font) - 2*G_SCALE, QString::number(unread_));
        }
        else
        {
            painter->drawText(notificationXOffset + MARGIN*G_SCALE, CommonGraphics::textHeight(*font) - 2*G_SCALE, numNotifications_);

        }
    }
}

void LogoNotificationsButton::setCountState(int countAll, int countUnread)
{
    numNotifications_ = QString::number(countAll);

    unread_ = countUnread;
    if (unread_ > 0)
    {
        backgroundFillColor_ =  FontManager::instance().getSeaGreenColor();
        backgroundOutlineColor_ = FontManager::instance().getSeaGreenColor();
        curBackgroundOpacity_ = 1;
        numberColor_ = FontManager::instance().getMidnightColor();
    }
    else
    {
        backgroundFillColor_ = Qt::transparent;
        backgroundOutlineColor_ = Qt::white;
        curBackgroundOpacity_ = OPACITY_ALL_READ;
        numberColor_ = Qt::white;
    }

    update();
}

void LogoNotificationsButton::onHoverEnter()
{
    update();
}

void LogoNotificationsButton::onHoverLeave()
{
    update();
}

}

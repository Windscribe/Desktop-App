#include "logonotificationsbutton.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {

LogoNotificationsButton::LogoNotificationsButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent)
    , curBackgroundOpacity_(OPACITY_ALL_READ), numNotifications_("0"), unread_(0), isPremium_(false)
{
    connect(this, &ClickableGraphicsObject::hoverEnter, this, &LogoNotificationsButton::onHoverEnter);
    connect(this, &ClickableGraphicsObject::hoverLeave, this, &LogoNotificationsButton::onHoverLeave);

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

    int notificationXOffset = 110*G_SCALE;

    // LOGO
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("LOGO");
    p->draw(0, 10*G_SCALE, painter);

    if (isPremium_) {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("PRO");
        p->draw(118*G_SCALE, 11*G_SCALE, painter);
        notificationXOffset += 43*G_SCALE;
    }

    QFont font = FontManager::instance().getFont(9, QFont::Bold);

    if (unread_ > 0 || hovered_) {
        // notification background
        painter->setFont(font);
        painter->setOpacity(curBackgroundOpacity_ * initOpacity);
        painter->setBrush(backgroundFillColor_);
        painter->setPen(backgroundOutlineColor_);
        painter->drawEllipse(QRect(notificationXOffset, 0, 14*G_SCALE, 14*G_SCALE));

        // notification number
        painter->setOpacity(initOpacity);
        painter->setPen(numberColor_);
        if (unread_ > 0) {
            painter->drawText(QRect(notificationXOffset, 0, 14*G_SCALE, 14*G_SCALE), Qt::AlignVCenter | Qt::AlignHCenter, QString::number(unread_));
        } else {
            painter->drawText(QRect(notificationXOffset, 0, 14*G_SCALE, 14*G_SCALE), Qt::AlignVCenter | Qt::AlignHCenter, numNotifications_);
        }
    }
}

void LogoNotificationsButton::setCountState(int countAll, int countUnread)
{
    numNotifications_ = QString::number(countAll);

    unread_ = countUnread;
    if (unread_ > 0) {
        backgroundFillColor_ = QColor(23, 233, 173, 52);
        backgroundOutlineColor_ = QColor(23, 233, 173, 26);
        curBackgroundOpacity_ = 1;
        numberColor_ = QColor(23, 233, 173);
    } else {
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

void LogoNotificationsButton::setIsPremium(bool isPremium)
{
    isPremium_ = isPremium;
    update();
}

}

#include "loginyesnobutton.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

const int HEIGHT = 40;

LoginYesNoButton::LoginYesNoButton(QString text, ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent),
    highlight_(false)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    width_ = LOGIN_WIDTH;
    height_ = HEIGHT;

    text_ = text;

    curTextOpacity_ = OPACITY_UNHOVER_TEXT;
    curIconOpacity_ = OPACITY_UNHOVER_ICON_TEXT;
    curTextPosX_ = MARGIN_UNHOVER_TEXT_PX;

    connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &LoginYesNoButton::onTextHoverOpacityChanged);
    connect(&iconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &LoginYesNoButton::onIconHoverOpacityChanged);
    connect(&textPosXAnimation_, &QVariantAnimation::valueChanged, this, &LoginYesNoButton::onTextHoverOffsetChanged);
    setClickable(true);
}

QRectF LoginYesNoButton::boundingRect() const
{
    return QRectF(0, 0, width_ * G_SCALE, height_ * G_SCALE);
}

void LoginYesNoButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    QFont font = FontManager::instance().getFont(16, QFont::Bold);
    painter->setFont(font);
    painter->setPen(Qt::white);

    painter->save();
    painter->translate(0, height_* G_SCALE/2);
    painter->setOpacity(curTextOpacity_ * initOpacity);
    painter->drawText(curTextPosX_* G_SCALE, 0, tr(text_.toStdString().c_str()));
    painter->restore();

    painter->save();
    painter->setOpacity(curIconOpacity_ * initOpacity);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("login/FRWRD_ARROW_ICON");
    p->draw(static_cast<int>(boundingRect().width()-WINDOW_MARGIN*2*G_SCALE), 5*G_SCALE, painter);
    painter->restore();

}

void LoginYesNoButton::setText(QString text)
{
    text_ = text;
}

bool LoginYesNoButton::isHighlighted()
{
    return highlight_;
}

void LoginYesNoButton::animateButtonHighlight()
{
    highlight_ = true;

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<int>(textPosXAnimation_, curTextPosX_, MARGIN_HOVER_TEXT_PX, ANIMATION_SPEED_FAST);

    emit activated();
}

void LoginYesNoButton::animateButtonUnhighlight()
{
    highlight_ = false;

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_UNHOVER_ICON_TEXT, ANIMATION_SPEED_FAST);
    startAnAnimation<int>(textPosXAnimation_, curTextPosX_, MARGIN_UNHOVER_TEXT_PX, ANIMATION_SPEED_FAST);
    setCursor(Qt::ArrowCursor);

    emit deactivated();
}

void LoginYesNoButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (clickable_)
    {
        animateButtonHighlight();
        setCursor(Qt::PointingHandCursor);
    }
}

void LoginYesNoButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    animateButtonUnhighlight();
}

void LoginYesNoButton::setOpacityByFactor(double newOpacityFactor)
{
    curTextOpacity_ = OPACITY_UNHOVER_TEXT * newOpacityFactor;
    curIconOpacity_ = OPACITY_UNHOVER_ICON_TEXT * newOpacityFactor;
    update();
}

void LoginYesNoButton::onTextHoverOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void LoginYesNoButton::onIconHoverOpacityChanged(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}

void LoginYesNoButton::onTextHoverOffsetChanged(const QVariant &value)
{
    curTextPosX_ = value.toInt();
    update();
}

}

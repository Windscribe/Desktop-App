#include "textlinkbutton.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QAbstractTextDocumentLayout>
#include <QCursor>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"

namespace EmergencyConnectWindow {

TextLinkButton::TextLinkButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent)
{
    curLinkOpacity_ = OPACITY_HIDDEN;
    curTextOpacity_ = OPACITY_HIDDEN;

    recalculateDimensions();

    connect(&linkOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextLinkButton::onLinkOpacityChanged);
    connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextLinkButton::onTextOpacityChanged);
    setClickable(true);
}

QRectF TextLinkButton::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void TextLinkButton::recalculateDimensions()
{
    prepareGeometryChange();
    QString text = tr("Access for  Windscribe.com  Only"); // double space on purpose
    QFont *font = FontManager::instance().getFont(12, false);
    QFontMetrics fm(*font);
    width_ = fm.boundingRect(text).width();
    height_ = fm.boundingRect(text).height();
    emit widthChanged();
}

void TextLinkButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity =  painter->opacity();

    painter->setOpacity(curTextOpacity_ * initOpacity);
    painter->setPen(Qt::white);
    QFont *font = FontManager::instance().getFont(12, false);
    painter->setFont(*font);

    QString accessForText = tr("Access for  "); // double space on purpose
    QFontMetrics fm(*font);
    int accessForWidth = fm.boundingRect(accessForText).width();
    painter->drawText(boundingRect(), accessForText);

    QString linkText = "Windscribe.com";
    QFont font2 = *font;
    font2.setUnderline(true);
    QFontMetrics fm2(font2);
    painter->setFont(font2);
    painter->setOpacity(curLinkOpacity_ * initOpacity);
    int linkWidth = fm2.boundingRect(linkText).width();
    painter->drawText(boundingRect().adjusted(accessForWidth,0,0,0), linkText);


    QString onlyText = tr("  Only"); // double space on purpose
    painter->setOpacity(curTextOpacity_ * initOpacity);
    painter->setFont(*font);
    painter->drawText(boundingRect().adjusted(accessForWidth + linkWidth,0,0,0), onlyText);


}

void TextLinkButton::animateShow(int animationSpeed)
{
    setClickable(true);

    startAnAnimation(textOpacityAnimation_, curTextOpacity_, OPACITY_UNHOVER_TEXT, animationSpeed);
    startAnAnimation(linkOpacityAnimation_, curLinkOpacity_, OPACITY_UNHOVER_TEXT, animationSpeed);

}

void TextLinkButton::animateHide(int animationSpeed)
{
    setClickable(false);

    startAnAnimation(textOpacityAnimation_, curTextOpacity_, OPACITY_HIDDEN, animationSpeed);
    startAnAnimation(linkOpacityAnimation_, curLinkOpacity_, OPACITY_HIDDEN, animationSpeed);

}

void TextLinkButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    recalculateDimensions();
}

void TextLinkButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (clickable_)
    {
        startAnAnimation(linkOpacityAnimation_, curLinkOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
}

void TextLinkButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    startAnAnimation(linkOpacityAnimation_, curLinkOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
}

void TextLinkButton::onLanguageChanged()
{
    //setScale();
}

void TextLinkButton::onLinkOpacityChanged(const QVariant &value)
{
    curLinkOpacity_ = value.toDouble();
    update();
}

void TextLinkButton::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

}

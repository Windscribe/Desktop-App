#include "texticonbutton.h"

#include <QCursor>
#include <QFontMetrics>
#include <QPainter>

#include "commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace CommonGraphics {

TextIconButton::TextIconButton(int spacerWidth, const QString text, const QString &imagePath, ScalableGraphicsObject *parent, bool bSetClickable)
  : ClickableGraphicsObject(parent), width_(0), maxWidth_(-1), height_(0), spacerWidth_(spacerWidth), iconPath_(imagePath), text_(text),
    curTextOpacity_(OPACITY_SIXTY), curIconOpacity_(OPACITY_SIXTY), fontDescr_(16, QFont::Bold), yOffset_(0)
{
    recalcHeight();
    recalcWidth();

    connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextIconButton::onTextOpacityChanged);
    connect(&iconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextIconButton::onIconOpacityChanged);

    setClickable(bSetClickable);
}

QRectF TextIconButton::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void TextIconButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QSharedPointer<IndependentPixmap> iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);

    // display text
    QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);
    painter->setOpacity(curTextOpacity_);
    painter->setFont(font);
    painter->setPen(Qt::white);
    int availableWidth = boundingRect().width() - iconPixmap->width() - spacerWidth_*2*G_SCALE;
    QString elidedText = text_;
    if (availableWidth < fm.horizontalAdvance(text_))
    {
        elidedText = fm.elidedText(text_, Qt::ElideRight, availableWidth, 0);
    }
    painter->drawText(boundingRect().adjusted(spacerWidth_*G_SCALE, yOffset_*G_SCALE, 0, 0), Qt::AlignVCenter, elidedText);

    // Right Arrow
    painter->setOpacity(curIconOpacity_);

    int y = 0;
    if (iconPixmap->height() < boundingRect().height()) {
        y = (boundingRect().height() - iconPixmap->height()) / 2;
    }
    iconPixmap->draw(boundingRect().width() - iconPixmap->width(), y, painter);
}

void TextIconButton::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
    recalcWidth();
    recalcHeight();
    update();
}

void TextIconButton::animateHide()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
}

void TextIconButton::animateShow()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
}

const QString TextIconButton::text()
{
    return text_;
}

void TextIconButton::setText(QString text)
{
    text_ = text;
    recalcWidth();
    update();
}

void TextIconButton::setMaxWidth(int width)
{
    maxWidth_ = width;
    recalcWidth();
    update();
}

int TextIconButton::getWidth()
{
    return width_;
}

int TextIconButton::getHeight()
{
    return height_;
}

void TextIconButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    recalcWidth();
    recalcHeight();
}

void TextIconButton::setOpacityByFactor(double opacityFactor)
{
    curTextOpacity_ = opacityFactor * OPACITY_SIXTY;
    curIconOpacity_ = opacityFactor * OPACITY_SIXTY;
    update();
}

void TextIconButton::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    if (clickable_)
    {
        startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
    emit hoverEnter();
}

void TextIconButton::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);

    emit hoverLeave();
}

void TextIconButton::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void TextIconButton::onIconOpacityChanged(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}

void TextIconButton::recalcWidth()
{
    QSharedPointer<IndependentPixmap> iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);

    QFont font = FontManager::instance().getFont(fontDescr_);
    int newWidth = std::ceil(iconPixmap->width() + 2*spacerWidth_*G_SCALE + CommonGraphics::textWidth(text_, font));

    if (maxWidth_ > 0 && newWidth > maxWidth_)
    {
        newWidth = maxWidth_;
    }

    if (newWidth != width_)
    {
        prepareGeometryChange();
        width_ = newWidth;
        emit widthChanged(newWidth);
    }
}

void TextIconButton::recalcHeight()
{
    QSharedPointer<IndependentPixmap> iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);

    QFont font = FontManager::instance().getFont(fontDescr_);

    const int newHeight = std::max(CommonGraphics::textHeight(font), iconPixmap->height());

    if (newHeight != height_)
    {
        prepareGeometryChange();
        height_ = newHeight;
        emit heightChanged(newHeight);
    }
}

void TextIconButton::setVerticalOffset(int offset)
{
    yOffset_ = offset;
}

void TextIconButton::setImagePath(const QString &imagePath)
{
    iconPath_ = imagePath;
    recalcWidth();
    recalcHeight();
    update();
}

void TextIconButton::setSpacerWidth(int spacerWidth)
{
    spacerWidth_ = spacerWidth;
    recalcWidth();
    update();
}

}

#include "textwrapiconbutton.h"

#include <QCursor>
#include <QFontMetrics>
#include <QPainter>

#include "commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace CommonGraphics {

TextWrapIconButton::TextWrapIconButton(int spacerWidth, const QString text, const QString &imagePath, ScalableGraphicsObject *parent, bool bSetClickable)
  : ClickableGraphicsObject(parent), width_(0), maxWidth_(-1), height_(0), spacerWidth_(spacerWidth), iconPath_(imagePath), text_(text),
    curTextOpacity_(OPACITY_SIXTY), curIconOpacity_(OPACITY_SIXTY), fontDescr_(16, QFont::Bold), yOffset_(0)
{
    recalcHeight();
    recalcWidth();

    connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextWrapIconButton::onTextOpacityChanged);
    connect(&iconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextWrapIconButton::onIconOpacityChanged);

    setClickable(bSetClickable);
}

QRectF TextWrapIconButton::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void TextWrapIconButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QSharedPointer<IndependentPixmap> iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);

    // Display text with word wrap on the left
    QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);
    painter->setOpacity(curTextOpacity_);
    painter->setFont(font);
    painter->setPen(Qt::white);
    
    // Calculate available width for text (total width minus icon width and spacers)
    int availableWidth = boundingRect().width() - iconPixmap->width() - spacerWidth_*3*G_SCALE;
    
    // Draw text with word wrapping aligned to the left
    QRect textRect(spacerWidth_*G_SCALE, yOffset_*G_SCALE, availableWidth, boundingRect().height() - yOffset_*G_SCALE);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text_);

    // Draw icon on the right
    painter->setOpacity(curIconOpacity_);

    int y = 0;
    if (iconPixmap->height() < boundingRect().height()) {
        y = (boundingRect().height() - iconPixmap->height()) / 2;
    }
    iconPixmap->draw(boundingRect().width() - iconPixmap->width(), y, painter);
}

void TextWrapIconButton::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
    recalcWidth();
    recalcHeight();
    update();
}

void TextWrapIconButton::animateHide()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
}

void TextWrapIconButton::animateShow()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
}

const QString TextWrapIconButton::text()
{
    return text_;
}

void TextWrapIconButton::setText(QString text)
{
    text_ = text;
    recalcHeight();
    update();
}

void TextWrapIconButton::setMaxWidth(int width)
{
    maxWidth_ = width;
    recalcWidth();
    recalcHeight();
    update();
}

int TextWrapIconButton::getWidth()
{
    return width_;
}

int TextWrapIconButton::getHeight()
{
    return height_;
}

void TextWrapIconButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    recalcWidth();
    recalcHeight();
}

void TextWrapIconButton::setOpacityByFactor(double opacityFactor)
{
    curTextOpacity_ = opacityFactor * OPACITY_SIXTY;
    curIconOpacity_ = opacityFactor * OPACITY_SIXTY;
    update();
}

void TextWrapIconButton::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    if (clickable_)
    {
        startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
    emit hoverEnter();
}

void TextWrapIconButton::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);

    emit hoverLeave();
}

void TextWrapIconButton::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void TextWrapIconButton::onIconOpacityChanged(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}

void TextWrapIconButton::recalcWidth()
{
    int newWidth = 0;

    if (maxWidth_ > 0)
    {
        newWidth = maxWidth_;
    }
    else
    {
        QSharedPointer<IndependentPixmap> iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);
        QFont font = FontManager::instance().getFont(fontDescr_);
        newWidth = std::ceil(iconPixmap->width() + 3*spacerWidth_*G_SCALE + CommonGraphics::textWidth(text_, font));
    }

    if (newWidth != width_)
    {
        prepareGeometryChange();
        width_ = newWidth;
        emit widthChanged(newWidth);
    }
}

void TextWrapIconButton::recalcHeight()
{
    QSharedPointer<IndependentPixmap> iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);
    QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);

    // Calculate text height with word wrapping
    int availableWidth = width_ - iconPixmap->width() - spacerWidth_*3*G_SCALE;
    QRect boundingRect = fm.boundingRect(0, 0, availableWidth, 0, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text_);
    int textHeight = boundingRect.height();

    const int newHeight = std::max(textHeight, iconPixmap->height());

    if (newHeight != height_)
    {
        prepareGeometryChange();
        height_ = newHeight;
        emit heightChanged(newHeight);
    }
}

void TextWrapIconButton::setVerticalOffset(int offset)
{
    yOffset_ = offset;
}

void TextWrapIconButton::setImagePath(const QString &imagePath)
{
    iconPath_ = imagePath;
    recalcWidth();
    recalcHeight();
    update();
}

}

#include "iconbuttonwidget.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"

namespace CommonWidgets {

IconButtonWidget::IconButtonWidget(QString imagePath, QWidget *parent) : IconButtonWidget(imagePath, "", parent)
{
}

IconButtonWidget::IconButtonWidget(const QString &imagePath, const QString &text, QWidget * parent) : QPushButton(parent)
  , font_(FontManager::instance().getFont(16, QFont::Bold))
{
    setImage(imagePath);
    setText(text);
    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &IconButtonWidget::onOpacityChanged);

    updateSize();
}

int IconButtonWidget::width()
{
    return width_;
}

int IconButtonWidget::height()
{
    return height_;
}

void IconButtonWidget::setText(const QString &text)
{
    text_ = text;
    updateSize();
}

void IconButtonWidget::setImage(const QString &imagePath)
{
    imagePath_ = imagePath;
    updateSize();
}

void IconButtonWidget::setFont(const QFont &font)
{
    font_ = font;
    updateSize();
}

void IconButtonWidget::setUnhoverHoverOpacity(double unhoverOpacity, double hoverOpacity)
{
    unhoverOpacity_ = unhoverOpacity;
    hoverOpacity_ = hoverOpacity;
}

void IconButtonWidget::animateOpacityChange(double targetOpacity)
{
    startAnAnimation<double>(opacityAnimation_, curOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void IconButtonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal initOpacity = painter.opacity();
    painter.setOpacity(curOpacity_ * initOpacity);

    if (!text_.isEmpty()) {
        painter.setPen(Qt::white);
        painter.setFont(font_);
        painter.drawText(QRect(0, 0, width_, height_), Qt::AlignVCenter, text_);
    }

    if (!imagePath_.isEmpty()) {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
        if (!p.isNull()) {
            p->draw(width() - p->width(), height()/2 - p->height()/2, &painter);
        }
    }
}

void IconButtonWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, hoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::PointingHandCursor);
    emit hoverEnter();
}

void IconButtonWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, unhoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::ArrowCursor);
    emit hoverLeave();
}

void IconButtonWidget::onOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

void IconButtonWidget::updateSize()
{
    int width = 0;
    int height = 0;

    if (!imagePath_.isEmpty()) {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
        if (!p.isNull()) {
            width =  p->width();
            height = p->height();
        }
    }

    if (!text_.isEmpty()) {
        QFontMetrics fm(font_);
        int textWidth = fm.horizontalAdvance(text_);
        int textHeight = fm.height();
        width += textWidth + (width > 0 ? 8*G_SCALE : 0);
        if (textHeight > height) {
            height = textHeight;
        }
    }

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        emit sizeChanged(width, height);
    }

    update();
}

}

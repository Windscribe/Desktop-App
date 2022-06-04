#include "textbuttonwidget.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace CommonWidgets {

TextButtonWidget::TextButtonWidget(QString text, QWidget * parent) : QPushButton(text, parent),
    unhoverOpacity_(OPACITY_UNHOVER_ICON_STANDALONE),
    hoverOpacity_(OPACITY_FULL),
    curOpacity_(OPACITY_UNHOVER_ICON_STANDALONE),
    fontDescr_(12, false)
{
    connect(this, SIGNAL(clicked()), SLOT(resetHoverState()));
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onOpacityChanged(QVariant)));
}

QSize TextButtonWidget::sizeHint() const
{
    return QSize(width_ * G_SCALE, height_ * G_SCALE);
}

void TextButtonWidget::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
}

void TextButtonWidget::setUnhoverHoverOpacity(double unhoverOpacity, double hoverOpacity)
{
    unhoverOpacity_ = unhoverOpacity;
    hoverOpacity_ = hoverOpacity;
}

void TextButtonWidget::animateOpacityChange(double targetOpacity)
{
    startAnAnimation<double>(opacityAnimation_, curOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void TextButtonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal initOpacity = painter.opacity();
    painter.setOpacity(curOpacity_ * initOpacity);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF rc(1 * G_SCALE, 1 * G_SCALE, (width_ - 2) * G_SCALE, (height_ - 2) * G_SCALE);
    painter.setPen(QPen(Qt::white, 2 * G_SCALE)); // thick line
    painter.drawRoundedRect(rc, arcsize_ * G_SCALE, arcsize_ * G_SCALE);

    const QString &buttonText = text();
    if (!buttonText.isEmpty()) {
        const QRectF rcText(0, 0, width_ * G_SCALE, (height_ - 1) * G_SCALE);
        const QFont *font = FontManager::instance().getFont(fontDescr_);
        painter.setFont(*font);
        painter.drawText(rcText, Qt::AlignCenter, buttonText);
    }
}

void TextButtonWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, hoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::PointingHandCursor);
}

void TextButtonWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    resetHoverState();
    setCursor(Qt::ArrowCursor);
}

void TextButtonWidget::resetHoverState()
{
    startAnAnimation(opacityAnimation_, curOpacity_, unhoverOpacity_, ANIMATION_SPEED_FAST);
}

void TextButtonWidget::onOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

}

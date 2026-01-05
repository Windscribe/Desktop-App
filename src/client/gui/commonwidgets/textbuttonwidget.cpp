#include "textbuttonwidget.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace CommonWidgets {

TextButtonWidget::TextButtonWidget(QString text, QWidget * parent) : QPushButton(text, parent),
    fontDescr_(14, QFont::Normal)
{
    updateWidth();
}

void TextButtonWidget::updateWidth()
{
    const QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);
    const int textWidth = fm.horizontalAdvance(text());
    width_ = (textWidth / G_SCALE) + 40;
}

QSize TextButtonWidget::sizeHint() const
{
    return QSize(width_ * G_SCALE, height_ * G_SCALE);
}

void TextButtonWidget::setText(const QString &text)
{
    QPushButton::setText(text);
    updateWidth();
}

void TextButtonWidget::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
    updateWidth();
}

void TextButtonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF rc(0, 0, width_ * G_SCALE, height_ * G_SCALE);
    const int radius = (height_ * G_SCALE) / 2;
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(rc, radius, radius);

    const QString &buttonText = text();
    if (!buttonText.isEmpty()) {
        const QRectF rcText(0, 0, width_ * G_SCALE, height_ * G_SCALE);
        const QFont font = FontManager::instance().getFont(fontDescr_);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(rcText, Qt::AlignCenter, buttonText);
    }
}

void TextButtonWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::PointingHandCursor);
}

void TextButtonWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
}

}

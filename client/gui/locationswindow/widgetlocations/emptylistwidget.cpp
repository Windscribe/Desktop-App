#include "emptylistwidget.h"

#include <QPainter>

#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"

namespace GuiLocations {

EmptyListWidget::EmptyListWidget(QWidget *parent) : QWidget(parent),
    textWidth_(0),
    textHeight_(0),
    button_(nullptr)
{
}

void EmptyListWidget::setText(const QString &text, int width)
{
    text_ = text;
    textWidth_ = width;
    update();
}

void EmptyListWidget::setIcon(const QString &icon)
{
    icon_ = icon;
    update();
}

void EmptyListWidget::setButton(const QString &buttonText)
{
    if (buttonText.isEmpty()) {
        delete button_;
        button_ = nullptr;
    } else {
        Q_ASSERT(button_ == nullptr);
        button_ = new CommonWidgets::TextButtonWidget(buttonText, this);
        button_->setFont(FontDescr(12, false));
        connect(button_, &CommonWidgets::TextButtonWidget::clicked, [this]() {
            emit clicked();
        });
        button_->show();
    }
    update();
}

void EmptyListWidget::updateScaling()
{
    updateButtonPos();
    update();
}

void EmptyListWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, FontManager::instance().getMidnightColor());

    const int kVerticalOffset = (button_ == nullptr) ? 0 : 16;
    if (!icon_.isEmpty()) {
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(icon_);
        pixmap->draw(width() / 2 - 16 * G_SCALE, height() / 2 - (kVerticalOffset + 32) * G_SCALE, &painter);
    }

    if (!text_.isEmpty()) {
        painter.save();
        QFont font = *FontManager::instance().getFont(12, false);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.setOpacity(0.5);
        QRect rc, rcb;
        const int wide = textWidth_ ? (textWidth_ * G_SCALE) :
            CommonGraphics::textWidth(text_, font);
        rc.setRect((width() - wide) / 2, height() / 2 - (kVerticalOffset - 16) * G_SCALE,
                   wide, height() );
        painter.drawText(rc, Qt::AlignHCenter | Qt::TextWordWrap, text_, &rcb);
        painter.restore();
        if (textHeight_ != rcb.height()) {
            textHeight_ = rcb.height();
            updateButtonPos();
        }
    }
}

void EmptyListWidget::updateButtonPos()
{
    if (!button_)
        return;

    const auto sizeHint = button_->sizeHint();
    const int xpos = (width() - sizeHint.width()) / 2;
    const int ypos = height() / 2 + textHeight_ + 16 * G_SCALE;
    button_->setGeometry(xpos, ypos, sizeHint.width(), sizeHint.height());
}

} // namespace GuiLocation

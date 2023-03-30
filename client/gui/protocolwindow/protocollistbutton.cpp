#include "protocollistbutton.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "utils/logger.h"
#include "dpiscalemanager.h"

namespace ProtocolWindow {

ProtocolListButton::ProtocolListButton(ScalableGraphicsObject *parent, const QString &text, const bool textOnly, const QColor &textColor)
    : BaseItem(parent, 24*G_SCALE, "", false, WINDOW_WIDTH - 72), text_(text), textButton_(nullptr), bubbleButton_(nullptr)
{
    if (textOnly) {
        textButton_ = new CommonGraphics::TextButton(text, FontDescr(14, true), textColor, true, this);
        connect(textButton_, &CommonGraphics::TextButton::clicked, this, &ProtocolListButton::clicked);
    } else {
        FontDescr fontDescr(14, false);
        QFont *font = FontManager::instance().getFont(fontDescr);
        QFontMetrics metrics(*font);

        int width = metrics.horizontalAdvance(text);
        int height = metrics.height();

        bubbleButton_ = new CommonGraphics::BubbleButtonBright(this, (width + 48*G_SCALE)/G_SCALE, (height + 20*G_SCALE)/G_SCALE, 20, 20);
        bubbleButton_->setFont(fontDescr);
        bubbleButton_->setText(text);
        connect(bubbleButton_, &CommonGraphics::BubbleButtonBright::clicked, this, &ProtocolListButton::clicked);
    }
    updatePositions();
}

void ProtocolListButton::paint(QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/)
{
}

void ProtocolListButton::updateScaling()
{
    BaseItem::updateScaling();
    updatePositions();
}

void ProtocolListButton::updatePositions()
{
    if (textButton_) {
        textButton_->setPos((WINDOW_WIDTH-72)/2*G_SCALE - textButton_->getWidth()/2, 0);
        setHeight(24*G_SCALE);
    } else if (bubbleButton_) {
        QFont *font = FontManager::instance().getFont(14, false);
        QFontMetrics metrics(*font);

        int width = metrics.horizontalAdvance(text_);
        int height = metrics.height();

        bubbleButton_->setPos((WINDOW_WIDTH-72)/2*G_SCALE - (width + 48*G_SCALE)/2, 0);
        setHeight(38*G_SCALE);
    }
    update();
}

} // namespace ProtocolWindow
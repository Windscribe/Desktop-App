#include "listbutton.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

ListButton::ListButton(ScalableGraphicsObject *parent, Style style, const QString &text)
    : BaseItem(parent, 24*G_SCALE, "", false, WINDOW_WIDTH - 72), style_(style), text_(text), textButton_(nullptr), bubbleButton_(nullptr)
{
    if (style == kText) {
        textButton_ = new TextButton(text, FontDescr(14, QFont::Bold), QColor(255, 255, 255), true, this);
        connect(textButton_, &TextButton::clicked, this, &ListButton::clicked);
    } else {
        FontDescr fontDescr(14, QFont::Normal);
        QFont font = FontManager::instance().getFont(fontDescr);
        QFontMetrics metrics(font);

        int width = metrics.horizontalAdvance(text);
        int height = metrics.height();

        bubbleButton_ = new BubbleButton(this,
                                         (style == kBright ? BubbleButton::kBright : BubbleButton::kDark),
                                         width/G_SCALE + 48,
                                         height/G_SCALE + 20,
                                         20);
        bubbleButton_->setFont(fontDescr);
        bubbleButton_->setText(text);
        connect(bubbleButton_, &BubbleButton::clicked, this, &ListButton::clicked);
    }
    updatePositions();
}

void ListButton::paint(QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/)
{
}

void ListButton::updateScaling()
{
    BaseItem::updateScaling();
    updatePositions();
}

void ListButton::setStyle(Style style)
{
    if (style_ == kText) {
        // Not implemented changing between text and bubble button styles
        return;
    }

    if (style == style_) {
        return;
    }

    if (style_ == kBright && style == kDark) {
        bubbleButton_->setStyle(BubbleButton::Style::kDark);
    } else if (style_ == kDark && style == kBright) {
        bubbleButton_->setStyle(BubbleButton::Style::kBright);
    }
    style_ = style;
    updatePositions();
}

void ListButton::updatePositions()
{
    if (style_ == kText) {
        textButton_->setPos((WINDOW_WIDTH-72)/2*G_SCALE - textButton_->getWidth()/2, 0);
        setHeight(24*G_SCALE);
    } else {
        QFont font = FontManager::instance().getFont(14,  QFont::Normal);
        QFontMetrics metrics(font);

        int width = metrics.horizontalAdvance(text_);
        bubbleButton_->setPos((WINDOW_WIDTH-72)/2*G_SCALE - (width + 48*G_SCALE)/2, 0);
        setHeight(38*G_SCALE);
    }
    update();
}

void ListButton::setText(const QString &text)
{
    if (style_ == kText) {
        textButton_->setText(text);
    } else {
        FontDescr fontDescr(14, QFont::Normal);
        QFont font = FontManager::instance().getFont(fontDescr);
        QFontMetrics metrics(font);
        int width = metrics.horizontalAdvance(text);

        bubbleButton_->setText(text);
        bubbleButton_->setWidth(width/G_SCALE + 48);
    }
    text_ = text;
    updatePositions();
}

void ListButton::hover() {
    if (style_ == kText) {
        textButton_->hover();
    } else {
        bubbleButton_->hover();
    }
}

void ListButton::unhover() {
    if (style_ == kText) {
        textButton_->unhover();
    } else {
        bubbleButton_->unhover();
    }
}

} // namespace CommonGraphics

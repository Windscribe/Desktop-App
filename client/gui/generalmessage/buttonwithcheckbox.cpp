#include "buttonwithcheckbox.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace GeneralMessageWindow {

ButtonWithCheckbox::ButtonWithCheckbox(ScalableGraphicsObject *parent, const CommonGraphics::BubbleButton::Style style, const QString &buttonText, const QString &checkboxText)
    : BaseItem(parent, 38*G_SCALE, "", true, WINDOW_WIDTH - 72), checkbox_(nullptr), buttonText_(buttonText), checkboxText_(checkboxText), showCheckbox_(false)
{
    setClickable(false);

    button_ = new CommonGraphics::BubbleButton(this, style, textWidth()/G_SCALE + 48, 38, 20);
    connect(button_, &CommonGraphics::BubbleButton::clicked, this, &ButtonWithCheckbox::clicked);
    connect(button_, &CommonGraphics::BubbleButton::hoverEnter, this, &ButtonWithCheckbox::buttonHoverEnter);
    connect(button_, &CommonGraphics::BubbleButton::hoverLeave, this, &ButtonWithCheckbox::buttonHoverLeave);
    button_->setFont(FontDescr(14, QFont::Normal));
    button_->setText(buttonText);

    setShowCheckbox(checkboxText);
    updateScaling();
}

void ButtonWithCheckbox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void ButtonWithCheckbox::updateScaling()
{
    setHeight(38*G_SCALE);

    if (checkbox_ == nullptr || !showCheckbox_) {
        // centered
        button_->setPos((WINDOW_WIDTH-72)/2*G_SCALE - (textWidth() + 48*G_SCALE)/2, 0);
    } else {
        // right-aligned
        button_->setPos((WINDOW_WIDTH-72)*G_SCALE - (textWidth() + 48*G_SCALE) - 12*G_SCALE, 0);
        // left-aligned
        checkbox_->setPos(12*G_SCALE, 38*G_SCALE/2 - checkbox_->boundingRect().height()/2);
    }
    update();
}

void ButtonWithCheckbox::setStyle(CommonGraphics::BubbleButton::Style style)
{
    button_->setStyle(style);
}

void ButtonWithCheckbox::setShowCheckbox(const QString &text)
{
    showCheckbox_ = !text.isEmpty();
    checkboxText_ = text;

    if (showCheckbox_) {
        if (checkbox_ != nullptr) {
            checkbox_->setText(checkboxText_);
            checkbox_->setVisible(true);
        } else {
            checkbox_ = new Checkbox(this, checkboxText_);
            checkbox_->setChecked(true);
        }
    } else {
        if (checkbox_ != nullptr) {
            checkbox_->setVisible(false);
        }
    }

    updateScaling();
}

int ButtonWithCheckbox::textWidth() const {
    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    QFontMetrics metrics(font);

    return metrics.horizontalAdvance(buttonText_);
}

void ButtonWithCheckbox::setText(const QString &text) {
    buttonText_ = text;
    button_->setText(text);
    button_->setWidth(textWidth()/G_SCALE + 48);
    updateScaling();
}

void ButtonWithCheckbox::hover()
{
    button_->hover();
}

void ButtonWithCheckbox::unhover()
{
    button_->unhover();
}

bool ButtonWithCheckbox::isCheckboxChecked() const
{
    return (checkbox_ && showCheckbox_ && checkbox_->isChecked());
}

} // namespace GeneralMessageWindow
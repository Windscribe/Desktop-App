
#include "checkbox.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"

Checkbox::Checkbox(ScalableGraphicsObject *parent, const QString &label) : ClickableGraphicsObject(parent), labelText_(label), checked_(false), hovering_(false)
{
    checkboxUnchecked_ = new IconButton(16, 16, "CHECKBOX_UNCHECKED", "", this);
    checkboxUnchecked_->setPos(0, 0);
    connect(checkboxUnchecked_, &IconButton::clicked, this, &Checkbox::onCheckboxChecked);
    connect(checkboxUnchecked_, &IconButton::hoverEnter, this, &Checkbox::onCheckboxHoverEnter);
    connect(checkboxUnchecked_, &IconButton::hoverLeave, this, &Checkbox::onCheckboxHoverLeave);

    checkboxChecked_ = new IconButton(16, 16, "CHECKBOX_CHECKED", "", this);
    checkboxChecked_->setUnhoverOpacity(OPACITY_FULL);
    checkboxChecked_->hover();
    checkboxChecked_->setVisible(false);
    checkboxChecked_->setPos(0, 0);
    connect(checkboxChecked_, &IconButton::clicked, this, &Checkbox::onCheckboxUnchecked);
    connect(checkboxChecked_, &IconButton::hoverEnter, this, &Checkbox::onCheckboxHoverEnter);
    connect(checkboxChecked_, &IconButton::hoverLeave, this, &Checkbox::onCheckboxHoverLeave);

    label_ = new CommonGraphics::TextButton(label, FontDescr(14, QFont::Normal), QColor(255, 255, 255), true, this);
    label_->setMarginHeight(0);
    label_->setUnhoverOnClick(false);
    label_->setPos(24*G_SCALE, 0);
    connect(label_, &CommonGraphics::TextButton::clicked, this, &Checkbox::onCheckboxToggled);
    connect(label_, &CommonGraphics::TextButton::hoverEnter, this, &Checkbox::onCheckboxHoverEnter);
    connect(label_, &CommonGraphics::TextButton::hoverLeave, this, &Checkbox::onCheckboxHoverLeave);

    setText(label);
}

void Checkbox::setChecked(bool checked)
{
    if (checked) {
        onCheckboxChecked();
    } else {
        onCheckboxUnchecked();
    }
}

void Checkbox::onCheckboxChecked()
{
    checked_ = true;
    checkboxChecked_->setVisible(true);
    checkboxUnchecked_->setVisible(false);
}

void Checkbox::onCheckboxUnchecked()
{
    checked_ = false;
    checkboxChecked_->setVisible(false);
    checkboxUnchecked_->setVisible(true);
}

void Checkbox::onCheckboxToggled()
{
    if (checked_) {
        onCheckboxUnchecked();
    } else {
        onCheckboxChecked();
    }
}

void Checkbox::onCheckboxHoverEnter()
{
    if (!hovering_) {
        hovering_ = true;
        checkboxChecked_->hover();
        checkboxUnchecked_->hover();
        label_->hover();
    }
}

void Checkbox::onCheckboxHoverLeave()
{
    if (hovering_) {
        hovering_ = false;
        checkboxChecked_->unhover();
        checkboxUnchecked_->unhover();
        label_->unhover();
    }
}

void Checkbox::setText(const QString &label)
{
    labelText_ = label;
    label_->setText(label);

    QFontMetrics fm(FontManager::instance().getFont(14,  QFont::Normal));
    width_ = fm.horizontalAdvance(labelText_);
    height_ = fm.height();
}

bool Checkbox::isChecked()
{
    return checked_;
}

QRectF Checkbox::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void Checkbox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}


#pragma once

#include "../commongraphics/baseitem.h"
#include "../commongraphics/bubblebutton.h"
#include "../commongraphics/checkbox.h"

namespace GeneralMessageWindow {

class ButtonWithCheckbox : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit ButtonWithCheckbox(ScalableGraphicsObject *parent, CommonGraphics::BubbleButton::Style, const QString &buttonText, const QString &checkboxText = "");
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setStyle(CommonGraphics::BubbleButton::Style style);
    void setText(const QString &text);
    void setShowCheckbox(const QString &text);
    void hover();
    void unhover();

    bool isCheckboxChecked() const;

signals:
    void buttonHoverEnter();
    void buttonHoverLeave();

private:
    CommonGraphics::BubbleButton *button_;
    Checkbox *checkbox_;
    QString buttonText_;
    QString checkboxText_;
    bool showCheckbox_;

    int textWidth() const;
};

} // namespace GeneralMessageWindow
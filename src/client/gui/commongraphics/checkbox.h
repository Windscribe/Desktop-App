#pragma once

#include "commongraphics/clickablegraphicsobject.h"
#include "commongraphics/commongraphics.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/textbutton.h"

class Checkbox: public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit Checkbox(ScalableGraphicsObject *parent, const QString &label);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(const QString &label);
    void setChecked(bool checked);
    bool isChecked();

private slots:
    void onCheckboxChecked();
    void onCheckboxUnchecked();
    void onCheckboxToggled();
    void onCheckboxHoverEnter();
    void onCheckboxHoverLeave();

private:
    IconButton *checkboxUnchecked_;
    IconButton *checkboxChecked_;
    CommonGraphics::TextButton *label_;
    bool checked_;
    bool hovering_;
    QString labelText_;

    int width_;
    int height_;
};

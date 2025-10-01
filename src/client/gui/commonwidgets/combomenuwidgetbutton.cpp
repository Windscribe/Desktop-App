#include "combomenuwidgetbutton.h"
#include "dpiscalemanager.h"

ComboMenuWidgetButton::ComboMenuWidgetButton(const QVariant &d)
    : QPushButton(), widthUnscaled_(0), heightUnscaled_(0)
{
    QString styleSheet = "QPushButton:hover { background-color: rgb(220, 220, 220) }";
    styleSheet += "QPushButton:checked { background-color: rgb(220,220,220) }";
    styleSheet += "QPushButton { background-color: white }";
    styleSheet += "QPushButton { border: none }";
    styleSheet += "QPushButton { padding-top: 5px; padding-bottom: 5px }";
    styleSheet += "QPushButton { padding-left: 10px; padding-right: 10px; }";
    styleSheet += "QPushButton { color: black }";
    setStyleSheet(styleSheet);

    data_ = d;
}

QVariant ComboMenuWidgetButton::data()
{
    return data_;
}

void ComboMenuWidgetButton::setWidthUnscaled(int width)
{
    widthUnscaled_= width;
    setFixedWidth(width * G_SCALE);
}

int ComboMenuWidgetButton::widthUnscaled()
{
    return widthUnscaled_;
}

void ComboMenuWidgetButton::setHeightUnscaled(int height)
{
    heightUnscaled_ = height;
    setFixedHeight(height * G_SCALE);
}

int ComboMenuWidgetButton::heightUnscaled()
{
    return heightUnscaled_;
}

void ComboMenuWidgetButton::enterEvent(QEnterEvent *e)
{
    emit hoverEnter();

    QWidget::enterEvent(e);
}

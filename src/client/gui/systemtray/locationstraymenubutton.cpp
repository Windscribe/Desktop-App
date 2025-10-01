#include "locationstraymenubutton.h"

#include <QPainter>
#include <QApplication>
#include <QStyleOption>

LocationsTrayMenuButton::LocationsTrayMenuButton(QWidget *parent) : QPushButton(parent), type_(TYPE_UP)
{
    setFlat(true);
}

void LocationsTrayMenuButton::setType(int type)
{
    type_ = type;
    update();
}

void LocationsTrayMenuButton::paintEvent(QPaintEvent *pEvent)
{
    QPushButton::paintEvent(pEvent);

    QPainter painter(this);
    QStyleOptionButton opt;
    opt.initFrom(this);
    QApplication::style()->drawPrimitive(type_ == TYPE_UP ? QStyle::PE_IndicatorArrowUp : QStyle::PE_IndicatorArrowDown, &opt, &painter);
}

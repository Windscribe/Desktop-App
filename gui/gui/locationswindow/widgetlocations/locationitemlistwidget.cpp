#include "locationitemlistwidget.h"

#include <QPainter>
#include <QDebug>

LocationItemListWidget::LocationItemListWidget(QWidget *parent) : QWidget(parent)
{
    qDebug() << "Constructed Location Item List Widget";
}

void LocationItemListWidget::paintEvent(QPaintEvent *event)
{
    // qDebug() << "Painting LocationItemListWidget: " << geometry();
    Q_UNUSED(event);
    QPainter painter(this);

    QRect redRect(    0, 0,    geometry().width(), 250);
    QRect blueRect(   0, 250,  geometry().width(), 250);
    QRect cyanRect(   0, 500,  geometry().width(), 250);
    QRect magentaRect(0, 750,  geometry().width(), 250);
    QRect greenRect(  0, 1000, geometry().width(), 250);
    QRect yellowRect( 0, 1250, geometry().width(), 250);

    painter.fillRect(redRect, Qt::red);
    painter.fillRect(blueRect, Qt::blue);
    painter.fillRect(cyanRect, Qt::cyan);
    painter.fillRect(magentaRect, Qt::magenta);
    painter.fillRect(greenRect, Qt::green);
    painter.fillRect(yellowRect, Qt::yellow);

    // qDebug() << "Yellow: " << yellowRect;
}

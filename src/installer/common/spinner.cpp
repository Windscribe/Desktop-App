#include "spinner.h"

#include <QPainter>
#include <QVariantAnimation>

#include "languagecontroller.h"

Spinner::Spinner(QWidget *parent) : QWidget(parent), angle_(0), isStarted_(false)
{
}

void Spinner::start()
{
    if (isStarted_) {
        return;
    }

    QVariantAnimation *animation = new QVariantAnimation(this);
    connect(animation, &QVariantAnimation::valueChanged, this, &Spinner::onAnimValueChanged);

    animation->setDuration(1000);
    animation->setStartValue(0);
    animation->setEndValue(360);
    animation->setLoopCount(-1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    isStarted_ = true;
}

void Spinner::onAnimValueChanged(const QVariant &value)
{
    angle_ = value.toInt();
    update();
}

void Spinner::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    QConicalGradient gradient;
    gradient.setCenter(QPointF(width()/2, height()/2));
    gradient.setAngle(-angle_);
    gradient.setColorAt(0, QColor(255, 255, 255, 255));
    gradient.setColorAt(1, QColor(255, 255, 255, 0));

    QPen pen(QBrush(gradient), 4);
    painter.setPen(pen);

    painter.drawArc(2, 2, width() - 4, height() - 4, -angle_*16, 270*16);
}

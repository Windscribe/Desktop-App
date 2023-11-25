#include "spinner.h"

#include <QPainter>
#include <QVariantAnimation>

#include "languagecontroller.h"

Spinner::Spinner(QWidget *parent) : QWidget(parent), angle_(0)
{
}

void Spinner::start()
{
    QVariantAnimation *animation = new QVariantAnimation(this);
    connect(animation, &QVariantAnimation::valueChanged, this, &Spinner::onAnimValueChanged);

    animation->setDuration(1000);
    animation->setStartValue(0);
    animation->setEndValue(360);
    animation->setLoopCount(-1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
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
    QPen pen(QColor(0x02, 0x0d, 0x1c));
    pen.setWidth(2);

    painter.setPen(pen);
    if (LanguageController::instance().isRtlLanguage()) {
        painter.drawArc(1, 1, width() - 2, height() - 2, angle_*16, 270*16);
    } else {
        painter.drawArc(1, 1, width() - 2, height() - 2, -angle_*16, 270*16);
    }
}

#include "togglebutton.h"

#include <QPainter>
#include "languagecontroller.h"

ToggleButton::ToggleButton(QWidget *parent) : QPushButton(parent), animValue_(0.0), checked_(false)
{
    setFixedSize(30, 16);
    setStyleSheet("QPushButton { background-color: transparent; }"
                  "QPushButton:hover { background-color: transparent; }"
                  "QPushButton:pressed { background-color: transparent; }");

    connect(this, &QPushButton::clicked, this, &ToggleButton::onClicked);
    connect(&anim_, &QVariantAnimation::valueChanged, this, &ToggleButton::onAnimValueChanged);
}

void ToggleButton::onClicked()
{
    if (checked_) {
        anim_.stop();
        anim_.setStartValue(animValue_);
        anim_.setEndValue(0.0);
        anim_.setDuration(100);
        anim_.start();
    } else {
        anim_.stop();
        anim_.setStartValue(animValue_);
        anim_.setEndValue(1.0);
        anim_.setDuration(100);
        anim_.start();
    }
    checked_ = !checked_;
    emit toggled(checked_);
}

void ToggleButton::onAnimValueChanged(const QVariant &value)
{
    animValue_ = value.toDouble();
    update();
}

void ToggleButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    // When "on", the color is 55ff8a, and when "off", the color is #ffffff
    painter.setBrush(QColor(255 - 170*animValue_, 255, 255 - 117*animValue_));
    painter.drawRoundedRect(0, 0, width(), height(), 2, 2);

    // The circle
    painter.setBrush(QColor(2, 13, 28));
    if (LanguageController::instance().isRtlLanguage()) {
        painter.drawEllipse(2 + 14*(1 - animValue_), 2, 12, 12);
    } else {
        painter.drawEllipse(2 + 14*animValue_, 2, 12, 12);
    }
}

void ToggleButton::toggle(bool on)
{
    if (checked_ == on) {
        return;
    }

    onClicked();
}

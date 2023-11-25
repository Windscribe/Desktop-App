#include "hoverbutton.h"

#include <QGraphicsOpacityEffect>
#include <QPainter>

#include "svgresources.h"
#include "themecontroller.h"

HoverButton::HoverButton(QWidget *parent) : QPushButton(parent), icon_(nullptr), hoverIcon_(nullptr)
{
    init();
}

HoverButton::HoverButton(QWidget *parent, const QString &imagePath) : QPushButton(parent), hoverIcon_(nullptr)
{
    init();

    icon_ = new QLabel(this);
    icon_->setPixmap(SvgResources::instance().pixmap(imagePath));
    icon_->show();
}

HoverButton::HoverButton(QWidget *parent, const QString &imagePath, const QString &hoverImagePath) : QPushButton(parent)
{
    init();

    icon_ = new QLabel(this);
    icon_->setPixmap(SvgResources::instance().pixmap(imagePath));
    icon_->show();

    hoverIcon_ = new QLabel(this);
    hoverIcon_->setPixmap(SvgResources::instance().pixmap(hoverImagePath));
    hoverIcon_->hide();
}

void HoverButton::init()
{
    text_ = nullptr;
    textSize_ = 12;
    hoverOpacity_ = 0.5;

    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_TranslucentBackground, true);

    setStyleSheet("QPushButton:pressed { background-color: transparent; }");

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(hoverOpacity_);
    anim.setTargetObject(effect);
    anim.setPropertyName("opacity");
    setGraphicsEffect(effect);
}

void HoverButton::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    if (icon_ && hoverIcon_) {
        icon_->hide();
        hoverIcon_->show();
    }
    anim.stop();
    anim.setStartValue(hoverOpacity_);
    anim.setEndValue(1.0);
    anim.setDuration(100);
    anim.start();
}

void HoverButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (icon_ && hoverIcon_) {
        hoverIcon_->hide();
        icon_->show();
    }
    anim.stop();
    anim.setStartValue(hoverOpacity_);
    anim.setEndValue(0.5);
    anim.setDuration(100);
    anim.start();
}

void HoverButton::setText(const QString &text)
{
    if (text_) {
        delete text_;
    }
    text_ = new QLabel(this);
    text_->setFont(ThemeController::instance().defaultFont(textSize_));
    text_->setText(text);
    text_->setStyleSheet("QLabel { color: #FFFFFF; }");

    QFontMetrics fm(ThemeController::instance().defaultFont(textSize_));
    setFixedWidth(std::max(fm.horizontalAdvance(text), icon_ ? icon_->width() : 0));

    if (icon_) {
        setFixedHeight(icon_->height() + 2 + fm.height());
        icon_->move(width()/2 - icon_->width()/2, 0);
        text_->move(width()/2 - fm.horizontalAdvance(text)/2, icon_->height() + 2);
    } else {
        QFontMetrics fm(ThemeController::instance().defaultFont(12));
        setFixedWidth(fm.horizontalAdvance(text));
        setFixedHeight(fm.height());
        text_->move(0, 0);
    }
}

void HoverButton::setTextSize(int size)
{
    textSize_ = size;
    if (text_) {
        setText(text_->text());
    }
}

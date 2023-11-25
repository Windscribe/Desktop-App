#include "alertwindow.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScreen>
#include <QWidget>

#include "languagecontroller.h"
#include "themecontroller.h"

AlertWindow::AlertWindow(QWidget *parent) : QWidget(parent)
{
    contents_ = new AlertWindowContents(this);
    connect(contents_, &AlertWindowContents::primaryButtonClicked, this, &AlertWindow::primaryButtonClicked);
    connect(contents_, &AlertWindowContents::escapeClicked, this, &AlertWindow::escapeClicked);
    connect(contents_, &AlertWindowContents::sizeChanged, this, &AlertWindow::onContentsSizeChanged);

    HoverButton *escapeButton = new HoverButton(this, ":/resources/ESCAPE_BUTTON.svg");
    escapeButton->setText(tr("ESC"));
    escapeButton->setTextSize(8);
    if (LanguageController::instance().isRtlLanguage()) {
        escapeButton->setGeometry(16, 16, 24, 40);
    } else {
        escapeButton->setGeometry(310, 16, 24, 40);
    }
    connect(escapeButton, &QPushButton::clicked, this, &AlertWindow::escapeClicked);
}

AlertWindow::~AlertWindow()
{
}

void AlertWindow::paintEvent(QPaintEvent *event)
{
    const ThemeController &themeCtrl = ThemeController::instance();
    QPainter painter(this);
    painter.fillRect(0, 0, 350, 350, ThemeController::instance().windowBackgroundColor());
}

void AlertWindow::setIcon(const QString &path)
{
    contents_->setIcon(path);
}

void AlertWindow::setTitle(const QString &title)
{
    contents_->setTitle(title);
}

void AlertWindow::setTitleSize(int px)
{
    contents_->setTitleSize(px);
}

void AlertWindow::setDescription(const QString &desc)
{
    contents_->setDescription(desc);
}

void AlertWindow::setPrimaryButton(const QString &text)
{
    contents_->setPrimaryButton(text);
}

void AlertWindow::setPrimaryButtonColor(const QColor &color)
{
    contents_->setPrimaryButtonColor(color);
}

void AlertWindow::setPrimaryButtonFontColor(const QColor &color)
{
    contents_->setPrimaryButtonFontColor(color);
}

void AlertWindow::setSecondaryButton(const QString &text)
{
    contents_->setSecondaryButton(text);
}

void AlertWindow::onContentsSizeChanged()
{
    int height = contents_->size().height();

    contents_->move(25, (size().height() - height)/2);
}

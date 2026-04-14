#include "alertwindow.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScreen>
#include <QWidget>

#include "hoverbutton.h"
#include "languagecontroller.h"
#include "themecontroller.h"
#include "utils.h"

AlertWindow::AlertWindow(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    height_ = parentWidget()->height();

    contents_ = new AlertWindowContents(this);
    connect(contents_, &AlertWindowContents::primaryButtonClicked, this, &AlertWindow::primaryButtonClicked);
    connect(contents_, &AlertWindowContents::escapeClicked, this, &AlertWindow::escapeClicked);
    connect(contents_, &AlertWindowContents::sizeChanged, this, &AlertWindow::onContentsSizeChanged);

    HoverButton *escapeButton = new HoverButton(this, ":/resources/ESCAPE_BUTTON.svg");
    escapeButton->setText(tr("ESC"));
    escapeButton->setTextAttributes(8, false, QFont::Medium);
    if (LanguageController::instance().isRtlLanguage()) {
        escapeButton->setGeometry(16, 16, 24, 40);
    } else {
        escapeButton->setGeometry(302, 16, 24, 40);
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

    int windowWidth = width();
    int windowHeight = height();

    QPixmap topPixmap = getBorderedPixmap("", windowWidth, 8, 8, true, false);
    painter.drawPixmap(0, 0, topPixmap);

    QPixmap bottomPixmap = getBorderedPixmap("", windowWidth, 8, 8, false, true);
    painter.drawPixmap(0, windowHeight - 8, bottomPixmap);

    if (windowHeight > 16) {
        QPixmap middlePixmap = getBorderedPixmap("", windowWidth, windowHeight - 16, 0, false, false);
        painter.drawPixmap(0, 8, middlePixmap);
    }
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

void AlertWindow::setDescriptionSize(int px)
{
    contents_->setDescriptionSize(px);
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
    if (height > parentWidget()->height()) {
        height_ = height;
    } else {
        height_ = parentWidget()->height();
    }
    setFixedHeight(height_);
}

void AlertWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit primaryButtonClicked();
    } else if (event->key() == Qt::Key_Escape) {
        emit escapeClicked();
    }
}

int AlertWindow::height() {
    return height_;
}

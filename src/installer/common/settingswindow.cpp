#include "settingswindow.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScreen>
#include <QWidget>

#include "languagecontroller.h"
#include "svgresources.h"
#include "themecontroller.h"
#include "togglebutton.h"
#include "utils.h"

SettingsWindow::SettingsWindow(QWidget *parent) : QWidget(parent), animValue_(0), path_(nullptr), factoryReset_(nullptr), createShortcut_(nullptr)
{
    setFocusPolicy(Qt::StrongFocus);

    QLabel *background = new QLabel(this);
    background->setPixmap(getBorderedPixmap(":/resources/background.png", 350, 350, 8));
    background->move(0, 0);

    QLabel *logo = new QLabel(this);
    logo->setPixmap(SvgResources::instance().pixmap(":/resources/WINDSCRIBE.svg"));
    logo->move(113, 24);

    QLabel *title = new QLabel(this);
    title->setFont(ThemeController::instance().defaultFont(16, QFont::Weight::Bold));
    title->setStyleSheet("QLabel { color: #FFFFFF; }");
    title->setText(tr("Install Settings"));
    QFontMetrics fm(title->font());
    title->move(175 - fm.horizontalAdvance(tr("Install Settings"))/2, 95);

    HoverButton *escapeButton = new HoverButton(this, ":/resources/CHECK_ICON.svg");
    escapeButton->setText(tr("OK"));
    escapeButton->setTextAttributes(10);
    escapeButton->move(175 - escapeButton->width()/2, 290);
    connect(escapeButton, &QPushButton::clicked, this, &SettingsWindow::escapeClicked);

#ifdef Q_OS_MACOS
    addDivider(200);

    addLabel(218, tr("Factory Reset"));
    ToggleButton *toggle = addToggle(218);
    connect(toggle, &ToggleButton::toggled, this, &SettingsWindow::factoryResetToggled);

    addDivider(250);
#else
    path_ = addLineEdit(143, "");
    connect(path_, &QLineEdit::editingFinished, this, &SettingsWindow::onEditingFinished);
    HoverButton *button = addIconButton(143, ":/resources/FOLDER_ICON.svg");
    connect(button, &HoverButton::clicked, this, &SettingsWindow::browseDirClicked);

    addDivider(175);

    addLabel(193, tr("Create shortcut"));
    createShortcut_ = addToggle(193);
    connect(createShortcut_, &ToggleButton::toggled, this, &SettingsWindow::createShortcutToggled);

    addDivider(225);

    addLabel(243, tr("Factory Reset"));
    factoryReset_ = addToggle(243);
    connect(factoryReset_, &ToggleButton::toggled, this, &SettingsWindow::factoryResetToggled);

    addDivider(275);
#endif

#ifdef Q_OS_MACOS
    HoverButton *closeButton = new HoverButton(this, ":/resources/MAC_CLOSE_DEFAULT.svg", ":/resources/MAC_CLOSE_HOVER.svg");
#else
    HoverButton *closeButton = new HoverButton(this, ":/resources/WINDOWS_CLOSE_DEFAULT.svg", ":/resources/WINDOWS_CLOSE_HOVER.svg");
#endif
    connect(closeButton, &QPushButton::clicked, this, &SettingsWindow::closeClicked);

#ifdef Q_OS_MACOS
    HoverButton *minimizeButton = new HoverButton(this, ":/resources/MAC_MINIMIZE_DEFAULT.svg", ":/resources/MAC_MINIMIZE_HOVER.svg");
#else
    HoverButton *minimizeButton = new HoverButton(this, ":/resources/WINDOWS_MINIMIZE_DEFAULT.svg", ":/resources/WINDOWS_MINIMIZE_HOVER.svg");
#endif
    connect(minimizeButton, &QPushButton::clicked, this, &SettingsWindow::minimizeClicked);

    if (LanguageController::instance().isRtlLanguage()) {
#ifdef Q_OS_MACOS
        closeButton->setGeometry(320, 16, 14, 14);
        minimizeButton->setGeometry(298, 16, 14, 14);
#else
        closeButton->setGeometry(16, 16, 28, 28);
        minimizeButton->setGeometry(46, 16, 28, 28);
#endif
    } else {
#ifdef Q_OS_MACOS
        closeButton->setGeometry(16, 16, 14, 14);
        minimizeButton->setGeometry(38, 16, 14, 14);
#else
        closeButton->setGeometry(306, 16, 28, 28);
        minimizeButton->setGeometry(262, 16, 28, 28);
#endif
    }

    connect(&anim_, &QVariantAnimation::valueChanged, this, &SettingsWindow::onAnimValueChanged);
    connect(&anim_, &QVariantAnimation::finished, this, &SettingsWindow::onAnimFinished);
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::setDimmedBackground(bool dimmed)
{
    if (dimmed) {
        anim_.stop();
        anim_.setStartValue(animValue_);
        anim_.setEndValue(192);
        anim_.setDuration(100);
        anim_.start();
    } else {
        anim_.stop();
        anim_.setStartValue(animValue_);
        anim_.setEndValue(0);
        anim_.setDuration(100);
        anim_.start();
    }
}

void SettingsWindow::onAnimValueChanged(const QVariant &value)
{
    animValue_ = value.toInt();
}

void SettingsWindow::onAnimFinished()
{
    emit animationFinished(animValue_ != 0);
}

QFrame *SettingsWindow::addDivider(int top)
{
    QFrame *divider = new QFrame(this);
    if (LanguageController::instance().isRtlLanguage()) {
        divider->setGeometry(0, top, 320, 2);
    } else {
        divider->setGeometry(30, top, 320, 2);
    }
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setLineWidth(2);
    divider->setStyleSheet("QFrame[frameShape=\"4\"] { color: #262626; }");

    return divider;
}

QLabel *SettingsWindow::addLabel(int top, const QString &text)
{
    QLabel *label = new QLabel(this);
    label->setFont(ThemeController::instance().defaultFont(12, QFont::Bold));
    label->setStyleSheet("QLabel { color: #FFFFFF; }");
    label->setText(text);
    if (LanguageController::instance().isRtlLanguage()) {
        label->move(328 - label->width(), top);
    } else {
        label->move(22, top);
    }

    return label;
}

ToggleButton *SettingsWindow::addToggle(int top)
{
    ToggleButton *toggle = new ToggleButton(this);
    if (LanguageController::instance().isRtlLanguage()) {
        toggle->move(22, top);
    } else {
        toggle->move(328 - toggle->width(), top);
    }
    return toggle;
}

QLineEdit *SettingsWindow::addLineEdit(int top, const QString &text)
{
    QLineEdit *lineedit = new QLineEdit(this);
    lineedit->setText(text);
    lineedit->setStyleSheet("QLineEdit { background-color: transparent; color: #66FFFFFF; border: none; }");
    if (LanguageController::instance().isRtlLanguage()) {
        lineedit->setLayoutDirection(Qt::RightToLeft);
        lineedit->setGeometry(54, top, 296, 16);
    } else {
        lineedit->setGeometry(22, top, 296, 16);
    }
    return lineedit;
}

HoverButton *SettingsWindow::addIconButton(int top, const QString &path)
{
    HoverButton *button = new HoverButton(this, path);
    if (LanguageController::instance().isRtlLanguage()) {
        button->setGeometry(22, top, 16, 16);
    } else {
        button->setGeometry(312, top, 16, 16);
    }
    return button;
}

void SettingsWindow::setFactoryReset(bool on)
{
    if (factoryReset_) {
        factoryReset_->toggle(on);
    }
}

void SettingsWindow::setCreateShortcut(bool on)
{
    if (createShortcut_) {
        createShortcut_->toggle(on);
    }
}

void SettingsWindow::setInstallPath(const QString &path)
{
    if (path_) {
        path_->setText(path);
    }
}

void SettingsWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit escapeClicked();
    }
}

void SettingsWindow::onEditingFinished()
{
    if (path_) {
        emit installPathChanged(path_->text());
    }
}
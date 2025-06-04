#include "initialwindow.h"

#include <QApplication>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QPainterPath>
#include <QWidget>
#include <spdlog/spdlog.h>

#include "languagecontroller.h"
#include "svgresources.h"
#include "themecontroller.h"
#include "utils.h"

InitialWindow::InitialWindow(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    QLabel *background = new QLabel(this);
    background->setPixmap(getRoundedRectPixmap(":/resources/background.png", 350, 350, 8));
    background->move(0, 0);

    QLabel *logo = new QLabel(this);
    logo->setPixmap(SvgResources::instance().pixmap(":/resources/WINDSCRIBE.svg"));
    logo->move(113, 24);

    installButton_ = new InstallButton(this);
    connect(installButton_, &QPushButton::clicked, this, &InitialWindow::onInstallClicked);
    installButton_->move(175 - installButton_->width()/2, 158);

    settingsButton_ = new HoverButton(this, ":/resources/SETTINGS_ICON.svg");
    settingsButton_->setGeometry(162, 285, 24, 24);
    connect(settingsButton_, &QPushButton::clicked, this, &InitialWindow::settingsClicked);

    eulaButton_ = new HoverButton(this);
    eulaButton_->setText(tr("Read EULA"));
    eulaButton_->setTextAttributes(12, true);
    eulaButton_->move(175 - eulaButton_->width()/2, 318);
    connect(eulaButton_, &QPushButton::clicked, this, &InitialWindow::onEulaClicked);

    progressDisplay_ = new ProgressDisplay(this);
    progressDisplay_->move(142, 141);
    progressDisplay_->hide();

#ifdef Q_OS_MACOS
    HoverButton *closeButton = new HoverButton(this, ":/resources/MAC_CLOSE_DEFAULT.svg", ":/resources/MAC_CLOSE_HOVER.svg");
#else
    HoverButton *closeButton = new HoverButton(this, ":/resources/WINDOWS_CLOSE_DEFAULT.svg", ":/resources/WINDOWS_CLOSE_HOVER.svg");
#endif
    connect(closeButton, &QPushButton::clicked, this, &InitialWindow::closeClicked);

#ifdef Q_OS_MACOS
    HoverButton *minimizeButton = new HoverButton(this, ":/resources/MAC_MINIMIZE_DEFAULT.svg", ":/resources/MAC_MINIMIZE_HOVER.svg");
#else
    HoverButton *minimizeButton = new HoverButton(this, ":/resources/WINDOWS_MINIMIZE_DEFAULT.svg", ":/resources/WINDOWS_MINIMIZE_HOVER.svg");
#endif
    connect(minimizeButton, &QPushButton::clicked, this, &InitialWindow::minimizeClicked);

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
}

InitialWindow::~InitialWindow()
{
}

void InitialWindow::onEulaClicked()
{
    QDesktopServices::openUrl(QUrl(QString("https://windscribe.com/terms/eula")));
}

void InitialWindow::setProgress(int progress)
{
    progressDisplay_->setProgress(progress);
    update();
}

void InitialWindow::onInstallClicked()
{
    settingsButton_->hide();
    eulaButton_->hide();
    installButton_->hide();
    progressDisplay_->show();
    progressDisplay_->setProgress(0);

    emit installClicked();
}

void InitialWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        onInstallClicked();
    } else if (event->key() == Qt::Key_Escape) {
        // if settingsButton_ is visible, then we're not installing yet
        if (settingsButton_->isVisible()) {
            emit closeClicked();
        }
    }
}

#include "initialwindow.h"

#include <QApplication>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QWidget>

#include "languagecontroller.h"
#include "svgresources.h"
#include "themecontroller.h"

InitialWindow::InitialWindow(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    QLabel *background = new QLabel(this);
    background->setPixmap(QPixmap(":/resources/background.png").scaled(350, 350));
    background->move(0, 0);

    QLabel *logo = new QLabel(this);
    logo->setPixmap(SvgResources::instance().pixmap(":/resources/WINDSCRIBE_ICON.svg"));
    logo->move(155, 16);

    installButton_ = new InstallButton(this);
    connect(installButton_, &QPushButton::clicked, this, &InitialWindow::onInstallClicked);
    installButton_->move(175 - installButton_->width()/2, 154);

    settingsButton_ = new HoverButton(this, ":/resources/SETTINGS_ICON.svg");
    settingsButton_->setGeometry(163, 285, 24, 24);
    connect(settingsButton_, &QPushButton::clicked, this, &InitialWindow::settingsClicked);

    eulaButton_ = new HoverButton(this);
    eulaButton_->setText(tr("Read EULA"));
    eulaButton_->move(175 - eulaButton_->width()/2, 322);
    connect(eulaButton_, &QPushButton::clicked, this, &InitialWindow::onEulaClicked);

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
        closeButton->setGeometry(328, 8, 14, 14);
        minimizeButton->setGeometry(306, 8, 14, 14);
#else
        closeButton->setGeometry(16, 16, 28, 28);
        minimizeButton->setGeometry(52, 16, 28, 28);
#endif
    } else {
#ifdef Q_OS_MACOS
        closeButton->setGeometry(8, 8, 14, 14);
        minimizeButton->setGeometry(28, 8, 14, 14);
#else
        closeButton->setGeometry(306, 16, 28, 28);
        minimizeButton->setGeometry(270, 16, 28, 28);
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
    installButton_->setProgress(progress);
    installButton_->move(175 - installButton_->width()/2, 154);
    update();
}

void InitialWindow::onInstallClicked()
{
    settingsButton_->hide();
    eulaButton_->hide();

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

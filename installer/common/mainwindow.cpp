#include "mainwindow.h"

#include <QApplication>
#include <QDirIterator>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QWidget>

#include "languagecontroller.h"
#include "themecontroller.h"

#ifdef Q_OS_WIN
#include "../windows/installer/installer/installer_utils.h"
#include "../windows/utils/applicationinfo.h"
#include "../windows/utils/path.h"
#include "../windows/utils/windscribepathcheck.h"
#endif

MainWindow::MainWindow(bool isAdmin, InstallerOptions &options) : QWidget(nullptr), options_(options)
{
    installerShim_ = &InstallerShim::instance();

    std::function<void()> callback = std::bind(&MainWindow::onInstallerCallback, this);
    installerShim_->setCallback(callback);

    qApp->setWindowIcon(QIcon(":/resources/Windscribe.ico"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(kWindowWidth, kWindowHeight);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // initialize language before creating UI elements
    LanguageController::instance();

    installerShim_->setFactoryReset(options_.factoryReset);
    installerShim_->setAutoStart(options_.autostart);
    installerShim_->setInstallDrivers(options_.installDrivers);
    std::wstring username = options_.username.toStdWString();
    std::wstring password = options_.password.toStdWString();
    installerShim_->setCredentials(username, password);

#ifdef Q_OS_WIN
    // Custom install path not currently supported on macOS.
    if (!options_.installPath.isEmpty()) {
        auto wpath = options_.installPath.toStdWString();
        if (PathCheck::isNeedAppendSubdirectory(wpath, installerShim_->installDir())) {
            wpath = Path::append(wpath, ApplicationInfo::name());
        }
        installerShim_->setInstallDir(wpath);
    }
#endif

    if (options_.silent) {
        startInstall();
        return;
    }

    QScreen *primaryScreen = qApp->primaryScreen();
    if (primaryScreen) {
        int screenWidth = primaryScreen->geometry().width();
        int screenHeight = primaryScreen->geometry().height();

        if (options_.centerX >= 0 && options_.centerY >= 0) {
            setGeometry(options_.centerX/primaryScreen->devicePixelRatio() - kWindowWidth/2,
                        options_.centerY/primaryScreen->devicePixelRatio() - kWindowHeight/2,
                        kWindowWidth,
                        kWindowHeight);
        } else {
            setGeometry((screenWidth - kWindowWidth) / 2, (screenHeight - kWindowHeight) / 2, kWindowWidth, kWindowHeight);
        }
    } else {
        setGeometry(0, 0, kWindowWidth, kWindowHeight);
    }

    initialWindow_ = new InitialWindow(this);
    initialWindow_->setGeometry(0, 0, width(), height());
    connect(initialWindow_, &InitialWindow::installClicked, this, &MainWindow::onInstallClicked);
    connect(initialWindow_, &InitialWindow::settingsClicked, this, &MainWindow::onSettingsClicked);
    connect(initialWindow_, &InitialWindow::minimizeClicked, this, &MainWindow::onMinimizeClicked);
    connect(initialWindow_, &InitialWindow::closeClicked, this, &MainWindow::onCloseClicked);

    settingsWindow_ = new SettingsWindow(this);
    settingsWindow_->setGeometry(0, 0, width(), height());
    connect(settingsWindow_, &SettingsWindow::escapeClicked, this, &MainWindow::onSettingsWindowEscapeClicked);
    connect(settingsWindow_, &SettingsWindow::browseDirClicked, this, &MainWindow::onChangeDirClicked);
    connect(settingsWindow_, &SettingsWindow::factoryResetToggled, this, &MainWindow::onSettingsWindowFactoryResetToggled);
    connect(settingsWindow_, &SettingsWindow::createShortcutToggled, this, &MainWindow::onSettingsWindowCreateShortcutToggled);
    connect(settingsWindow_, &SettingsWindow::installPathChanged, this, &MainWindow::onSettingsWindowInstallPathChanged);
    connect(settingsWindow_, &SettingsWindow::animationFinished, this, &MainWindow::onSettingsWindowAnimFinished);
    connect(settingsWindow_, &SettingsWindow::minimizeClicked, this, &MainWindow::onMinimizeClicked);
    connect(settingsWindow_, &SettingsWindow::closeClicked, this, &MainWindow::onCloseClicked);
    settingsWindow_->setCreateShortcut(installerShim_->isCreateShortcutEnabled());
    settingsWindow_->setFactoryReset(installerShim_->isFactoryResetEnabled());
    settingsWindow_->setInstallPath(QString::fromStdWString(installerShim_->installDir()));
    settingsWindow_->hide();

    alertWindow_ = new AlertWindow(this);
    alertWindow_->setGeometry(0, 0, width(), height());
    alertWindow_->hide();

    if (!isAdmin) {
        showError(tr("Installation failed"),
                  tr("You don't have sufficient permissions to run this application. Administrative privileges are required to install Windscribe."),
                  true);
        return;
    }

    if (options_.updating) {
        onInstallClicked();
    } else {
        initialWindow_->show();
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::onInstallClicked()
{
#ifdef Q_OS_WIN
    // Display the warning every time if a user is manually upgrading and they're using a custom folder.
    // We'll only pester users doing an in-app upgrade if they are upgrading from a version not containing
    // this security check.
    if (isWarnUserCustomPath()) {
        if (!options_.updating || InstallerUtils::installedAppVersionLessThan(L"2.10.10")) {
            showCustomPathWarning(true);
            return;
        }
    }
#endif

    startInstall();
}

void MainWindow::onSettingsClicked()
{
    initialWindow_->hide();
    settingsWindow_->show();
    settingsWindow_->setFocus();
    settingsWindow_->setDimmedBackground(true);
}

void MainWindow::onSettingsWindowEscapeClicked()
{
    settingsWindow_->setDimmedBackground(false);
}

void MainWindow::onSettingsWindowFactoryResetToggled(bool on)
{
    installerShim_->setFactoryReset(on && !options_.updating);
}

void MainWindow::onSettingsWindowCreateShortcutToggled(bool on)
{
    installerShim_->setCreateShortcut(on);
}

void MainWindow::onSettingsWindowInstallPathChanged(const QString &path)
{
    setInstallPath(path);
}

void MainWindow::onSettingsWindowAnimFinished(bool dimmed)
{
    if (!dimmed) {
        settingsWindow_->hide();
        initialWindow_->show();
        initialWindow_->setFocus();
    }
}

void MainWindow::onInstallerCallback()
{
    switch (installerShim_->state()) {
    case InstallerShim::STATE_INIT:
        break;
    case InstallerShim::STATE_EXTRACTING:
    case InstallerShim::STATE_EXTRACTED:
        // No UI created when running in silent mode.
        if (initialWindow_) {
            QMetaObject::invokeMethod(initialWindow_, "setProgress", Qt::QueuedConnection, Q_ARG(int, installerShim_->progress()));
        }
        break;
    case InstallerShim::STATE_CANCELED:
    case InstallerShim::STATE_LAUNCHED:
        qApp->exit();
        break;
    case InstallerShim::STATE_FINISHED:
        installerShim_->finish();
        break;
    case InstallerShim::STATE_ERROR:
        QString errorMsg;
        InstallerShim::INSTALLER_ERROR error = installerShim_->lastError();
        if (error == InstallerShim::ERROR_PERMISSION) {
            errorMsg = tr("The installation was cancelled. Administrator privileges are required to install the application.");
        } else if (error == InstallerShim::ERROR_KILL) {
            errorMsg = tr("Windscribe is running and could not be closed. Please close the application manually and try again.");
        } else if (error == InstallerShim::ERROR_CONNECT_HELPER) {
            errorMsg = tr("The installer could not connect to the privileged helper tool. Please try again.");
        } else if (error == InstallerShim::ERROR_DELETE) {
            errorMsg = tr("An existing installation of Windscribe could not be removed. Please uninstall the application manually and try again.");
        } else if (error == InstallerShim::ERROR_UNINSTALL) {
            errorMsg = tr("The uninstaller for the existing installation of Windscribe could not be found. Please uninstall the application manually and try again.");
        } else if (error == InstallerShim::ERROR_MOVE_CUSTOM_DIR) {
            errorMsg = tr("The installation folder contains data which could not be uninstalled. Please uninstall the application manually and try again.");
        } else {
            errorMsg = tr("The installation could not be completed successfully. Please contact our Technical Support.");
        }
        // No UI created when running in silent mode.
        if (options_.silent) {
            // On Windows this will go to the system debugger (e.g. Debug View app).
            qDebug() << errorMsg;
            qApp->exit();
        }
        else {
            QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection, Q_ARG(QString, tr("Installation failed")),
                                      Q_ARG(QString, errorMsg), Q_ARG(bool, true));
        }
        break;
    }
}

void MainWindow::onMinimizeClicked()
{
    showMinimized();
}

void MainWindow::onCloseClicked()
{
    if (exiting_) {
        qApp->exit();
    } else {
        showExitPrompt();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && mousePressed_) {
        this->move(event->globalPosition().toPoint() - dragPosition_);
        event->accept();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition_ = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
        mousePressed_ = true;
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && mousePressed_) {
        mousePressed_ = false;
    }
}

void MainWindow::onChangeDirClicked()
{
#ifdef Q_OS_WIN
    std::wstring title = tr("Select a folder in the list below and click OK.").toStdWString();
    std::wstring installFolder = InstallerUtils::selectInstallFolder((HWND)winId(), title, installerShim_->installDir());
    if (!installFolder.empty()) {
        setInstallPath(QString::fromStdWString(installFolder));
    }
#endif
}

void MainWindow::showError(const QString &title, const QString &desc, bool fatal)
{
    if (!alertWindow_) {
        return;
    }

    fatalError_ = fatal;

    disconnect(alertWindow_, nullptr, nullptr, nullptr);

    alertWindow_->setIcon(":/resources/ERROR.svg");
    alertWindow_->setTitle(title);
    alertWindow_->setTitleSize(16);
    alertWindow_->setDescription(desc);
    alertWindow_->setPrimaryButton(tr("OK"));
    alertWindow_->setPrimaryButtonColor(ThemeController::instance().primaryButtonColor());
    alertWindow_->setPrimaryButtonFontColor(ThemeController::instance().primaryButtonFontColor());
    alertWindow_->setSecondaryButton("");

    connect(alertWindow_, &AlertWindow::primaryButtonClicked, this, &MainWindow::onAlertWindowPrimaryButtonClicked);
    connect(alertWindow_, &AlertWindow::escapeClicked, this, &MainWindow::onAlertWindowEscapeClicked);

    alertWindow_->show();
    alertWindow_->setFocus();
}

void MainWindow::showExitPrompt()
{
    if (!alertWindow_) {
        return;
    }

    exiting_ = true;

    disconnect(alertWindow_, nullptr, nullptr, nullptr);

    alertWindow_->setIcon(":/resources/SHUTDOWN_ICON.svg");
    alertWindow_->setTitle(tr("Quit Windscribe Installer?"));
    alertWindow_->setTitleSize(14);
    alertWindow_->setDescription("");
    alertWindow_->setPrimaryButton(tr("Quit"));
    alertWindow_->setPrimaryButtonColor(QColor(0xFF, 0xFF, 0xFF, 0x23));
    alertWindow_->setPrimaryButtonFontColor(QColor(0xFF, 0xFF, 0xFF));
    alertWindow_->setSecondaryButton(tr("Cancel"));

    connect(alertWindow_, &AlertWindow::primaryButtonClicked, this, &MainWindow::onAlertWindowPrimaryButtonClicked);
    connect(alertWindow_, &AlertWindow::escapeClicked, this, &MainWindow::onAlertWindowEscapeClicked);

    alertWindow_->show();
    alertWindow_->setFocus();
}

void MainWindow::showCustomPathWarning(bool installing)
{
    // Custom install path not currently supported on macOS.
#ifdef Q_OS_WIN
    if (!alertWindow_) {
        return;
    }

    disconnect(alertWindow_, nullptr, nullptr, nullptr);

    // Using slightly smaller font sizes to ensure the 'Cancel' button is not pushed off the bottom
    // of the screen.
    alertWindow_->setIcon(":/resources/WARNING.svg");
    alertWindow_->setTitle(tr("Security Warning"));
    alertWindow_->setTitleSize(14);
    alertWindow_->setDescription(
        tr("Installation to a custom folder may allow an attacker to tamper with the Windscribe application."
           " To ensure the security of the application, and your system, we strongly recommend you install"
           " to the default location in the 'Program Files' folder. Click OK to continue with the custom"
           " folder or Cancel to use the default location."));
    alertWindow_->setDescriptionSize(12);
    alertWindow_->setPrimaryButton(tr("OK"));
    alertWindow_->setPrimaryButtonColor(ThemeController::instance().primaryButtonColor());
    alertWindow_->setPrimaryButtonFontColor(ThemeController::instance().primaryButtonFontColor());
    alertWindow_->setSecondaryButton(tr("Cancel"));

    if (installing) {
        connect(alertWindow_, &AlertWindow::primaryButtonClicked, this, [this] {
            showActiveWindowAfterAlert();
            startInstall();
        });
        connect(alertWindow_, &AlertWindow::escapeClicked, this, [this] {
            installerShim_->setInstallDir(ApplicationInfo::defaultInstallPath());
            showActiveWindowAfterAlert();
            startInstall();
        });
    }
    else {
        connect(alertWindow_, &AlertWindow::primaryButtonClicked, this, [this] { showActiveWindowAfterAlert(); });
        connect(alertWindow_, &AlertWindow::escapeClicked, this, [this] {
            // User opted not to use the custom install path they selected.  Revert to the default install path.
            const auto defaultPath = ApplicationInfo::defaultInstallPath();
            installerShim_->setInstallDir(defaultPath);
            settingsWindow_->setInstallPath(QString::fromStdWString(defaultPath));
            lastCustomPathWarning_.clear();
            showActiveWindowAfterAlert();
        });
    }

    alertWindow_->show();
    alertWindow_->setFocus();
#else
    Q_UNUSED(installing)
#endif
}

void MainWindow::onAlertWindowPrimaryButtonClicked()
{
    showActiveWindowAfterAlert();
    if (fatalError_ || exiting_) {
        qApp->exit();
    }
}

void MainWindow::onAlertWindowEscapeClicked()
{
    showActiveWindowAfterAlert();
    exiting_ = false;
    if (fatalError_) {
        qApp->exit();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // If already at the prompt, just exit, otherwise show prompt
    if (!exiting_) {
        event->ignore();
    }

    if (!installing_) {
        showExitPrompt();
    }
}

void MainWindow::setInstallPath(const QString &path)
{
    // Custom install path not currently supported on macOS.
#ifdef Q_OS_WIN
    auto wpath = path.toStdWString();

    if (!Path::isOnSystemDrive(wpath)) {
        const auto defaultPath = ApplicationInfo::defaultInstallPath();
        installerShim_->setInstallDir(defaultPath);
        settingsWindow_->setInstallPath(QString::fromStdWString(defaultPath));
        showError(tr("Invalid path"),
                  tr("The specified installation path is not on the system drive. To ensure the security of the application, and your system, it must be installed on the same drive as Windows. The installation folder has been reset to the default."),
                  false);
        return;
    }

    if (PathCheck::isNeedAppendSubdirectory(wpath, installerShim_->installDir())) {
        wpath = Path::append(wpath, ApplicationInfo::name());
    }

    installerShim_->setInstallDir(wpath);
    settingsWindow_->setInstallPath(QString::fromStdWString(wpath));

    if (isWarnUserCustomPath()) {
        lastCustomPathWarning_ = wpath;
        showCustomPathWarning(false);
    }
#endif
}

void MainWindow::showActiveWindowAfterAlert()
{
    alertWindow_->hide();
    if (settingsWindow_->isVisible()) {
        settingsWindow_->setFocus();
    } else {
        initialWindow_->setFocus();
    }
}

void MainWindow::startInstall()
{
    if (!installing_) {
        installing_ = true;
        installerShim_->start(options_.updating);
    }
}

bool MainWindow::isWarnUserCustomPath() const
{
#ifdef Q_OS_WIN
    const auto installPath = installerShim_->installDir();
    return !Path::equivalent(lastCustomPathWarning_, installPath) && !Path::isSystemProtected(installPath);
#else
    return false;
#endif
}

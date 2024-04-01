#include "mainwindow.h"

#include <QApplication>
#include <QDirIterator>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QWidget>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj_core.h>
#include "../windows/utils/applicationinfo.h"
#include "../windows/utils/path.h"
#include "../windows/utils/windscribepathcheck.h"
#endif
#include "languagecontroller.h"
#include "themecontroller.h"

MainWindow::MainWindow(bool isAdmin, InstallerOptions &options) : QWidget(nullptr),
    options_(options), mousePressed_(false), fatalError_(false), exiting_(false), installing_(false)
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
    std::wstring installPath = options_.installPath.toStdWString();
    installerShim_->setInstallDir(installPath);
    installerShim_->setAutoStart(options_.autostart);
    installerShim_->setInstallDrivers(options_.installDrivers);
    std::wstring username = options_.username.toStdWString();
    std::wstring password = options_.password.toStdWString();
    installerShim_->setCredentials(username, password);

    if (!options_.silent) {
        QScreen *primaryScreen = qApp->primaryScreen();
        if (primaryScreen) {
            int screenWidth = primaryScreen->geometry().width();
            int screenHeight = primaryScreen->geometry().height();

            if (options_.centerX >= 0 && options_.centerY >= 0) {
                setGeometry(options_.centerX - kWindowWidth/2, options_.centerY - kWindowHeight/2, kWindowWidth, kWindowHeight);
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
        connect(alertWindow_, &AlertWindow::primaryButtonClicked, this, &MainWindow::onAlertWindowPrimaryButtonClicked);
        connect(alertWindow_, &AlertWindow::escapeClicked, this, &MainWindow::onAlertWindowEscapeClicked);
        alertWindow_->hide();
    }

    if (!isAdmin) {
        showError(tr("Installation failed"),
                  tr("You don't have sufficient permissions to run this application. Administrative privileges are required to install Windscribe."),
                  true);
        return;
    }

    if (options_.updating || options_.silent) {
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
    if (!installing_) {
        installing_ = true;
        installerShim_->start(options_.updating);
    }
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

void MainWindow::setProgress(int progress)
{
    QMetaObject::invokeMethod(initialWindow_, "setProgress", Qt::QueuedConnection, Q_ARG(int, progress));
}

void MainWindow::onInstallerCallback()
{
    switch (installerShim_->state()) {
        case InstallerShim::STATE_INIT:
            break;
        case InstallerShim::STATE_EXTRACTING:
            QMetaObject::invokeMethod(initialWindow_, "setProgress", Qt::QueuedConnection, Q_ARG(int, installerShim_->progress()));
            break;
        case InstallerShim::STATE_CANCELED:
        case InstallerShim::STATE_LAUNCHED:
            qApp->exit();
            break;
        case InstallerShim::STATE_FINISHED:
            installerShim_->finish();
            break;
        case InstallerShim::STATE_ERROR:
            InstallerShim::INSTALLER_ERROR error = installerShim_->lastError();
            if (error == InstallerShim::ERROR_PERMISSION) {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("The installation was cancelled. Administrator privileges are required to install the application.")),
                                          Q_ARG(bool, true));
            } else if (error == InstallerShim::ERROR_KILL) {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("Windscribe is running and could not be closed. Please close the application manually and try again.")),
                                          Q_ARG(bool, true));
            } else if (error == InstallerShim::ERROR_CONNECT_HELPER) {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("The installer could not connect to the privileged helper tool. Please try again.")),
                                          Q_ARG(bool, true));
            } else if (error == InstallerShim::ERROR_DELETE) {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("An existing installation of Windscribe could not be removed. Please uninstall the application manually and try again.")),
                                          Q_ARG(bool, true));
            } else if (error == InstallerShim::ERROR_UNINSTALL) {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("The uninstaller for the existing installation of Windscribe could not be found. Please uninstall the application manually and try again.")),
                                          Q_ARG(bool, true));
            } else if (error == InstallerShim::ERROR_MOVE_CUSTOM_DIR) {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("The installation folder contains data which could not be uninstalled. Please uninstall the application manually and try again.")),
                                          Q_ARG(bool, true));
            } else {
                QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection,
                                          Q_ARG(QString, tr("Installation failed")),
                                          Q_ARG(QString, tr("The installation could not be completed successfully. Please contact our Technical Support.")),
                                          Q_ARG(bool, true));
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

#ifdef Q_OS_WIN
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
        std::wstring tmp = (const wchar_t*)lpData;
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}
#endif

void MainWindow::onChangeDirClicked()
{
#ifdef Q_OS_WIN
    std::wstring path_param = installerShim_->installDir();
    std::wstring title = tr("Select a folder in the list below and click OK.").toStdWString();

    BROWSEINFO bi = { 0 };
    bi.hwndOwner = (HWND)winId();
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)path_param.c_str();

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        //get the name of the folder and put it in path
        TCHAR path[MAX_PATH];
        SHGetPathFromIDList(pidl, path);
        CoTaskMemFree(pidl);

        std::wstring final_path = path;

        if (PathCheck::isNeedAppendSubdirectory(path, path_param)) {
            final_path = Path::append(path, ApplicationInfo::name());
        }

        setInstallPath(QString::fromStdWString(final_path));
    }
#endif
}

void MainWindow::showError(const QString &title, const QString &desc, bool fatal)
{
    if (options_.silent) {
        return;
    }

    alertWindow_->setIcon(":/resources/ERROR.svg");
    alertWindow_->setTitle(title);
    alertWindow_->setTitleSize(16);
    alertWindow_->setDescription(desc);
    alertWindow_->setPrimaryButton(tr("OK"));
    alertWindow_->setPrimaryButtonColor(ThemeController::instance().primaryButtonColor());
    alertWindow_->setPrimaryButtonFontColor(ThemeController::instance().primaryButtonFontColor());
    alertWindow_->setSecondaryButton("");
    fatalError_ = fatal;
    alertWindow_->show();
    alertWindow_->setFocus();
}

void MainWindow::showExitPrompt()
{
    if (options_.silent) {
        return;
    }

    alertWindow_->setIcon(":/resources/SHUTDOWN_ICON.svg");
    alertWindow_->setTitle(tr("Quit Windscribe Installer?"));
    alertWindow_->setTitleSize(14);
    alertWindow_->setDescription("");
    alertWindow_->setPrimaryButton(tr("Quit"));
    alertWindow_->setPrimaryButtonColor(QColor(0xFF, 0xFF, 0xFF, 0x23));
    alertWindow_->setPrimaryButtonFontColor(QColor(0xFF, 0xFF, 0xFF));
    alertWindow_->setSecondaryButton(tr("Cancel"));

    exiting_ = true;
    alertWindow_->show();
    alertWindow_->setFocus();
}

void MainWindow::onAlertWindowPrimaryButtonClicked()
{
    alertWindow_->hide();
    if (settingsWindow_->isVisible()) {
        settingsWindow_->setFocus();
    } else {
        initialWindow_->setFocus();
    }
    if (fatalError_ || exiting_) {
        qApp->exit();
    }
}

void MainWindow::onAlertWindowEscapeClicked()
{
    alertWindow_->hide();
    if (settingsWindow_->isVisible()) {
        settingsWindow_->setFocus();
    } else {
        initialWindow_->setFocus();
    }
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
    std::wstring wpath = path.toStdWString();
    bool err = installerShim_->setInstallDir(wpath);
    if (err) {
        // This function only called for Windows, so we know which error it is
        showError(tr("Invalid path"),
                  tr("The specified installation path is not on the system drive. To ensure the security of the application, and your system, it must be installed on the same drive as Windows. The installation folder has been reset to the default."),
                  false);
    } else {
        settingsWindow_->setInstallPath(path);
    }
}

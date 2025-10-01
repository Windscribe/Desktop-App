#include <QWidget>

#include "alertwindow.h"
#include "initialwindow.h"
#include "installer_shim.h"
#include "options.h"
#include "settingswindow.h"

#define kWindowWidth 350
#define kWindowHeight 350

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(bool isAdmin, InstallerOptions &options);
    ~MainWindow();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void showError(const QString &title, const QString &desc, bool fatal);
    void onInstallClicked();
    void onSettingsClicked();
    void onSettingsWindowEscapeClicked();
    void onSettingsWindowAnimFinished(bool dimmed);
    void onSettingsWindowFactoryResetToggled(bool on);
    void onSettingsWindowCreateShortcutToggled(bool on);
    void onSettingsWindowInstallPathChanged(const QString &path);
    void onChangeDirClicked();
    void onMinimizeClicked();
    void onCloseClicked();
    void onAlertWindowPrimaryButtonClicked();
    void onAlertWindowEscapeClicked();
    void onInstallerCallback();

private:
    const InstallerOptions options_;
    InitialWindow *initialWindow_ = nullptr;
    SettingsWindow *settingsWindow_ = nullptr;
    AlertWindow *alertWindow_ = nullptr;
    InstallerShim *installerShim_ = nullptr;
    bool mousePressed_ = false;
    bool fatalError_ = false;
    bool exiting_ = false;
    bool installing_ = false;
    QPoint dragPosition_;
    std::wstring lastCustomPathWarning_;

    bool isWarnUserCustomPath() const;
    void showActiveWindowAfterAlert();
    void showExitPrompt();
    void setInstallPath(const QString &path);
    void showCustomPathWarning(bool installing);
    void startInstall();
    void showAlertWindow();
};

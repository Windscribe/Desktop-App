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

    void setProgress(int progress);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

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
    InstallerOptions options_;
    InitialWindow *initialWindow_;
    SettingsWindow *settingsWindow_;
    AlertWindow *alertWindow_;
    InstallerShim *installerShim_;
    bool mousePressed_;
    bool fatalError_;
    bool exiting_;
    bool installing_;
    QPoint dragPosition_;

    void showExitPrompt();
    void setInstallPath(const QString &path);
};

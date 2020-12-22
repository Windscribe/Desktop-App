#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>
#include "dpiscalemanager.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "application/windscribeapplication.h"
#include "graphicresources/imageresourcessvg.h"

#ifdef Q_OS_WIN
    #include "application/preventmultipleinstances_win.h"
    #include "utils/scaleutils_win.h"
    #include "utils/crashhandler.h"
#else
    #include "utils/macutils.h"
#endif

void applyScalingFactor(qreal ldpi, MainWindow &mw);

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // prevent multiple instances of process for Windows
    PreventMultipleInstances_win multipleInstances;
    if (!multipleInstances.lock())
    {
        return 0;
    }
#endif


    // set Qt plugin library paths for release build
#ifndef QT_DEBUG
    #ifdef Q_OS_WIN
        // For Windows an empty list means searching plugins in the executable folder
        QCoreApplication::setLibraryPaths(QStringList());
    #else
        QStringList pluginsPath;
        pluginsPath << MacUtils::getBundlePath() + "/Contents/PlugIns";
        QCoreApplication::setLibraryPaths(pluginsPath);
    #endif
#endif

    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName("gui");
    Debug::CrashHandler::instance().bindToProcess();
#endif

#ifdef Q_OS_MAC
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    WindscribeApplication a(argc, argv);

    bool guiInstanceAlreadyRunning = Utils::giveFocusToGui();
    if (guiInstanceAlreadyRunning)
    {
        qCDebug(LOG_BASIC) << "GUI appears to be running -- quitting";
        return 0;
    }

    // These values are used for QSettings by default
    a.setOrganizationName("Windscribe");
    a.setApplicationName("Windscribe2");

    a.setApplicationDisplayName("Windscribe");


    Logger::instance().install("gui", true);

    qCDebug(LOG_BASIC) << "App start time:" << QDateTime::currentDateTime().toString();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();

#if defined Q_OS_MAC
    if (!MacUtils::isOsVersion10_11_or_greater())
    {
        QMessageBox msgBox;
        msgBox.setText( QObject::tr("Windscribe is not compatible with your version of OSX. Please upgrade to 10.11+") );
        msgBox.setIcon( QMessageBox::Information );
        msgBox.exec();
        return 0;
    }
#endif
    a.setStyle("fusion");

    DpiScaleManager::instance();    // init dpi scale manager

    // init and show tray icon
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(*IconManager::instance().getDisconnectedIcon());
    trayIcon.show();

    while (trayIcon.geometry().isEmpty())
    {
        a.processEvents();
    }
    qCDebug(LOG_BASIC) << "Tray Icon geometry:" << trayIcon.geometry();

    MainWindow w(trayIcon);
    w.showAfterLaunch();

#ifdef Q_OS_WIN
    multipleInstances.unlock();
#endif
    int ret = a.exec();
    ImageResourcesSvg::instance().finishGracefully();
    return ret;
}


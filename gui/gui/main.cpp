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
#else
    #include "utils/macutils.h"
#endif


#ifdef Q_OS_MAC
    void handler_sigterm(int signum)
    {
        Q_UNUSED(signum);
        qCDebug(LOG_BASIC) << "SIGTERM signal received";
        /*if (g_MainWindow)
        {
            g_MainWindow->doClose(NULL, true);
        }*/
        exit(0);
    }
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

    // clear Qt plugin library paths
    QCoreApplication::setLibraryPaths(QStringList());

    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

#ifdef Q_OS_MAC
    signal(SIGTERM, handler_sigterm);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    WindscribeApplication a(argc, argv);

    bool guiInstanceAlreadyRunning = Utils::giveFocusToGui();
    if (guiInstanceAlreadyRunning)
    {
        qCDebug(LOG_BASIC) << "GUI appears to be running -- quitting";
        return 0;
    }

    a.setApplicationName("Windscribe");
    a.setOrganizationName("Windscribe");

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

    MainWindow w;
    w.show();

    DpiScaleManager::instance().init(&w);
    QTimer::singleShot(0, [&w](){
        w.updateScaling();
    });



#ifdef Q_OS_WIN
    multipleInstances.unlock();
#endif

    return a.exec();
}


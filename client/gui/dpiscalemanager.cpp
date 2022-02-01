#include "dpiscalemanager.h"
#include <QWidget>
#include <QWindow>
#include <QApplication>
#include "utils/logger.h"

static QRect GetGeometryForScreen(const QScreen *screen)
{
#ifdef Q_OS_WIN
    // Account for the taskbar on Windows, since it is always on top, and effectively reduces the
    // available screen geometry for applications.
    return screen->availableGeometry();
#else
    return screen->geometry();
#endif
}

bool DpiScaleManager::setMainWindow(QWidget *mainWindow)
{
    mainWindow_ = mainWindow;
    connect(mainWindow->window()->windowHandle(), SIGNAL(screenChanged(QScreen *)), SLOT(onWindowScreenChanged(QScreen*)));

    const auto screenList = qApp->screens();
    for (QScreen *screen : screenList)
    {
        connect(screen, SIGNAL(logicalDotsPerInchChanged(qreal)), SLOT(onLogicalDotsPerInchChanged(qreal)));
    }

    QScreen *screen = mainWindow->window()->windowHandle()->screen();
    curGeometry_ = GetGeometryForScreen(screen);

    if (screen->logicalDotsPerInch() != curDPI_ || !qFuzzyCompare(curDevicePixelRatio_, screen->devicePixelRatio()))
    {
        curDPI_ = mainWindow->window()->windowHandle()->screen()->logicalDotsPerInch();
    #ifdef Q_OS_WIN
        curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
    #else
        // for Mac curScale always == 1
        curScale_ = 1;
    #endif

        curDevicePixelRatio_ = (int)mainWindow->window()->windowHandle()->screen()->devicePixelRatio();
        // curDevicePixelRatio_ should be 1 or 2 (not tested for other values)
        Q_ASSERT(qFuzzyCompare(curDevicePixelRatio_, 1.0) || qFuzzyCompare(curDevicePixelRatio_, 2.0));
        return  true;
    }
    else
    {
        return false;
    }

}

double DpiScaleManager::scaleOfScreen(const QScreen *screen) const
{
    qreal scale;
#ifdef Q_OS_WIN
    const qreal dpi = screen->logicalDotsPerInch();
    scale = (double)dpi / (double)LOWEST_LDPI;
#else
    // for Mac curScale always == 1
    Q_UNUSED(screen);
    scale = 1;
#endif
    return scale;
}

void DpiScaleManager::onWindowScreenChanged(QScreen *screen)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::onScreenChanged, new DPI" << screen->logicalDotsPerInch();
    if (screen->logicalDotsPerInch() != curDPI_ || !qFuzzyCompare(curDevicePixelRatio_, screen->devicePixelRatio()))
    {
#ifdef Q_OS_WIN
        curDPI_ = screen->logicalDotsPerInch();
        curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
#endif
        curDevicePixelRatio_ = (int)screen->devicePixelRatio();
        // curDevicePixelRatio_ should be 1 or 2 (not tested for other values)
        Q_ASSERT(qFuzzyCompare(curDevicePixelRatio_, 1.0) || qFuzzyCompare(curDevicePixelRatio_, 2.0));
        emit scaleChanged(curScale_);
    }
    qCDebug(LOG_BASIC) << "DpiScaleManager :: new screen: " << screen;
    curGeometry_ = GetGeometryForScreen(screen);
    emit newScreen(screen);
}

void DpiScaleManager::onLogicalDotsPerInchChanged(qreal dpi)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::onScreenChanged, new DPI" << dpi;
    if (mainWindow_->window()->windowHandle()->screen() == sender())
    {
        onWindowScreenChanged((QScreen *)sender());
    }
}

DpiScaleManager::DpiScaleManager() : QObject(nullptr), curDevicePixelRatio_(1),
                                     curGeometry_(GetGeometryForScreen(qApp->primaryScreen())),
                                     mainWindow_(nullptr)
{   
    curDPI_ = qApp->primaryScreen()->logicalDotsPerInch();

#ifdef Q_OS_WIN
    curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
#else
    // for Mac curScale always == 1
    curScale_ = 1;
#endif

    curGeometry_ = GetGeometryForScreen(qApp->primaryScreen());
    curDevicePixelRatio_ = (int)qApp->primaryScreen()->devicePixelRatio();
    qCDebug(LOG_BASIC) << "DpiScaleManager::constructor -> DPI:" << curDPI_ << "; scale:" << curScale_ << "; devicePixelRatio:" << curDevicePixelRatio_;
}

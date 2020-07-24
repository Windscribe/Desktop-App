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

void DpiScaleManager::init(QWidget *mainWindow)
{
    mainWindow_ = mainWindow;
    connect(mainWindow->window()->windowHandle(), SIGNAL(screenChanged(QScreen *)), SLOT(onWindowScreenChanged(QScreen*)));
    connect(qApp, SIGNAL(screenAdded(QScreen*)), SLOT(onScreenAdded(QScreen*)));
    connect(qApp, SIGNAL(screenRemoved(QScreen*)), SLOT(onScreenRemoved(QScreen*)));


    curDPI_ = mainWindow->window()->windowHandle()->screen()->logicalDotsPerInch();
#ifdef Q_OS_WIN
    curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
#else
    // for Mac curScale always == 1
    curScale_ = 1;
#endif
    curGeometry_ = GetGeometryForScreen(mainWindow->window()->windowHandle()->screen());

    curDevicePixelRatio_ = (int)mainWindow->window()->windowHandle()->screen()->devicePixelRatio();
    // curDevicePixelRatio_ should be 1 or 2 (not tested for other values)
    Q_ASSERT(qFuzzyCompare(curDevicePixelRatio_, 1.0) || qFuzzyCompare(curDevicePixelRatio_, 2.0));

    appCurScreen_ = mainWindow->window()->windowHandle()->screen();

    foreach (QScreen *screen, qApp->screens())
    {
        connect(screen, SIGNAL(logicalDotsPerInchChanged(qreal)), SLOT(onLogicalDotsPerInchChanged(qreal)));
    }
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
    curGeometry_ = GetGeometryForScreen(screen);
}

void DpiScaleManager::onLogicalDotsPerInchChanged(qreal dpi)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::onScreenChanged, new DPI" << dpi;
    if (mainWindow_->window()->windowHandle()->screen() == sender())
    {
        onWindowScreenChanged((QScreen *)sender());
    }
}

void DpiScaleManager::onScreenAdded(QScreen *screen)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::onScreenAdded, DPI" << screen->logicalDotsPerInch();
    screens_ << screen;
    emit screenConfigurationChanged(screens_);
}

void DpiScaleManager::onScreenRemoved(QScreen *screen)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::onScreenRemoved, DPI" << screen->logicalDotsPerInch();
    screens_.remove(screen);
    emit screenConfigurationChanged(screens_);
}

DpiScaleManager::DpiScaleManager() : QObject(NULL)
{
    curDevicePixelRatio_ = 1;
#ifdef Q_OS_WIN
    curDPI_ = qApp->primaryScreen()->logicalDotsPerInch();
    curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
#else
    curScale_ = 1;
#endif
    curGeometry_ = GetGeometryForScreen(qApp->primaryScreen());
}

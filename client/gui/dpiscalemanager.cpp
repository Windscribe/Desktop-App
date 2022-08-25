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

DpiScaleManager::DpiScaleManager()
    : QObject(nullptr),
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
    curDevicePixelRatio_ = qApp->primaryScreen()->devicePixelRatio();
    qCDebug(LOG_BASIC) << "DpiScaleManager::constructor -> DPI:" << curDPI_ << "; scale:" << curScale_ << "; devicePixelRatio:" << curDevicePixelRatio_;
}

bool DpiScaleManager::setMainWindow(QWidget *mainWindow)
{
    mainWindow_ = mainWindow;
    connect(mainWindow->window()->windowHandle(), &QWindow::screenChanged, this, &DpiScaleManager::onWindowScreenChanged);

    for (QScreen *screen : qApp->screens()) {
        connect(screen, &QScreen::logicalDotsPerInchChanged, this, &DpiScaleManager::onLogicalDotsPerInchChanged);
    }

    // Detect addition of a new screen while the app is running so we can monitor its logicalDotsPerInchChanged signal.
    // We don't need to monitor removal of a screen, as Qt will automatically disconnect our connection to that
    // screen's logicalDotsPerInchChanged signal.
    connect(qApp, &QGuiApplication::screenAdded, this, &DpiScaleManager::onScreenAdded);

    QScreen *screen = mainWindow_->screen();

    if (screen == nullptr) {
        qCDebug(LOG_BASIC) << "DpiScaleManager::setMainWindow - WARNING mainWindow_->screen() is null";
        return false;
    }

    curGeometry_ = GetGeometryForScreen(screen);

    if (screen->logicalDotsPerInch() != curDPI_ || !qFuzzyCompare(curDevicePixelRatio_, screen->devicePixelRatio()))
    {
        curDPI_ = screen->logicalDotsPerInch();
    #ifdef Q_OS_WIN
        curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
    #else
        // for Mac curScale always == 1
        curScale_ = 1;
    #endif

        curDevicePixelRatio_ = screen->devicePixelRatio();
        qCDebug(LOG_BASIC) << "DpiScaleManager::setMainWindow -> DPI:" << curDPI_ << "; scale:" << curScale_ << "; devicePixelRatio:" << curDevicePixelRatio_;
        return  true;
    }

    qCDebug(LOG_BASIC) << "DpiScaleManager::setMainWindow -> no DPI or pixel ratio changes";
    return false;
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
    qCDebug(LOG_BASIC) << "DpiScaleManager::onScreenChanged - new screen: " << screen;
    update(screen);
    emit newScreen(screen);
}

void DpiScaleManager::onLogicalDotsPerInchChanged(qreal dpi)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::onLogicalDotsPerInchChanged, new DPI" << dpi;
    if (mainWindow_->screen() == sender()) {
        update(mainWindow_->screen());
    }
}

int DpiScaleManager::curDevicePixelRatio() const
{
    // curDevicePixelRatio_ should be 1 or 2 (not tested for other values)
    int ratio = qRound(curDevicePixelRatio_);
    if (ratio < 1) {
        ratio = 1;
    }
    else if (ratio > 2) {
        ratio = 2;
    }
    return ratio;
}

QRect DpiScaleManager::curScreenGeometry() const
{
    // Due to a Qt bug in Windows, the screen geometry is incorrect when onLogicalDotsPerInchChanged is called.
    // QScreen::geometry() and QScreen::availableGeometry() will return the geometry of the screen as it was
    // BEFORE the scale change.  For example, if we change the scaling factor on a 1080P display from 150% to
    // the recommended non-scaled 100%, calling QScreen::geometry() in onLogicalDotsPerInchChanged gives us
    // the geometry of the display at the 150% scale factor.
    // Therefore, we'll delay getting the geometry until someone actually requires it, and we're only going
    // to rely on the cached value if the mainwindow_ is not available, which shouldn't happen in practice.
    if (mainWindow_) {
        return GetGeometryForScreen(mainWindow_->screen());
    }

    qCDebug(LOG_BASIC) << "WARNING - DpiScaleManager::curScreenGeometry() returning cached geometry, which may be incorrect:" << curGeometry_;
    return curGeometry_;
}

void DpiScaleManager::onScreenAdded(QScreen *screen)
{
    connect(screen, &QScreen::logicalDotsPerInchChanged, this, &DpiScaleManager::onLogicalDotsPerInchChanged);
}

void DpiScaleManager::update(QScreen *screen)
{
    qCDebug(LOG_BASIC) << "DpiScaleManager::update - DPI, devicePixelRatio" << screen->logicalDotsPerInch() << screen->devicePixelRatio();
    if (screen->logicalDotsPerInch() != curDPI_ || !qFuzzyCompare(curDevicePixelRatio_, screen->devicePixelRatio()))
    {
#ifdef Q_OS_WIN
        curDPI_ = screen->logicalDotsPerInch();
        curScale_ = (double)curDPI_ / (double)LOWEST_LDPI;
#endif
        curDevicePixelRatio_ = screen->devicePixelRatio();
        emit scaleChanged(curScale_);
    }
    curGeometry_ = GetGeometryForScreen(screen);
}

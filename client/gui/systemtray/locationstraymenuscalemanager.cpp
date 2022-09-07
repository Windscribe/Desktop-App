#include "locationstraymenuscalemanager.h"
#include <QApplication>
#include <QScreen>
#include "utils/ws_assert.h"

void LocationsTrayMenuScaleManager::setTrayIconGeometry(const QRect &geometry)
{
    screen_ = qApp->screenAt(geometry.topLeft());
    WS_ASSERT(screen_ != nullptr);
    if (!screen_)
    {
        screen_ = qApp->primaryScreen();
    }

    if (screen_)
    {
        #ifdef Q_OS_WIN
            int curDPI = screen_->logicalDotsPerInch();
            scale_ = (double)curDPI / (double)LOWEST_LDPI;
        #else
            // for Mac curScale always == 1
            scale_ = 1;
        #endif
    }
    else
    {
        scale_ = 1.0;
    }
}

double LocationsTrayMenuScaleManager::scale() const
{
    return scale_;
}

QScreen *LocationsTrayMenuScaleManager::screen()
{
    return screen_;
}

LocationsTrayMenuScaleManager::LocationsTrayMenuScaleManager() : scale_(1.0)
{
    screen_ = qApp->primaryScreen();
}

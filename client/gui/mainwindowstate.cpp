#include "mainwindowstate.h"
#include "utils/logger.h"

bool MainWindowState::isActive() const
{
    return isActive_;
}

void MainWindowState::setActive(bool isActive)
{
    if (isActive_ != isActive)
    {
        isActive_ = isActive;
        //qCDebug(LOG_BASIC) << "SetActive:" << isActive_;
        emit isActiveChanged(isActive_);
    }
}

MainWindowState::MainWindowState() : isActive_(true)
{

}

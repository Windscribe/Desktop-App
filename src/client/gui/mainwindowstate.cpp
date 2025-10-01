#include "mainwindowstate.h"

bool MainWindowState::isActive() const
{
    return isActive_;
}

void MainWindowState::setActive(bool isActive)
{
    if (isActive_ != isActive)
    {
        isActive_ = isActive;
        emit isActiveChanged(isActive_);
    }
}

MainWindowState::MainWindowState() : isActive_(true)
{

}

#include "showingdialogstate.h"

bool ShowingDialogState::isCurrentlyShowingExternalDialog() const
{
    return currentlyShowingExternalDialog_;
}

void ShowingDialogState::setCurrentlyShowingExternalDialog(bool bShowing)
{
    currentlyShowingExternalDialog_ = bShowing;
}

ShowingDialogState::ShowingDialogState() : currentlyShowingExternalDialog_(false)
{

}

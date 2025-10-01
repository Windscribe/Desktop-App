#pragma once

// singleton, global indicator for external dialog state. When an external dialog is used, we should set the flag.
class ShowingDialogState
{

public:
    static ShowingDialogState &instance()
    {
        static ShowingDialogState s;
        return s;
    }

    bool isCurrentlyShowingExternalDialog() const;
    void setCurrentlyShowingExternalDialog(bool bShowing);

private:
    ShowingDialogState();

private:
    bool currentlyShowingExternalDialog_;
};

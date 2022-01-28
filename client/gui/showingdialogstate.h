#ifndef SHOWINGDIALOGSTATE_H
#define SHOWINGDIALOGSTATE_H

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

#endif // SHOWINGDIALOGSTATE_H

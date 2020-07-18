#ifndef BFECONTROLLER_WIN_H
#define BFECONTROLLER_WIN_H

class IHelper;
class QWidget;

// manage BFE (Base filtering engine) service
class BFEController_win
{
public:
    static BFEController_win &instance()
    {
        static BFEController_win i;
        return i;
    }

    bool checkAndEnableBFE(IHelper *helper);
    bool isBFEEnabled();

private:
    BFEController_win();

    void enableBFE(IHelper *helper);
};

#endif // BFECONTROLLER_WIN_H

#ifndef BFE_SERVICE_WIN_H
#define BFE_SERVICE_WIN_H

class IHelper;
class QWidget;

// manage BFE (Base filtering engine) service
class BFE_Service_win
{
public:
    static BFE_Service_win &instance()
    {
        static BFE_Service_win s;
        return s;
    }

    bool checkAndEnableBFE(IHelper *helper);
    bool isBFEEnabled();

private:
    BFE_Service_win();

    void enableBFE(IHelper *helper);
};

#endif // BFE_SERVICE_WIN_H

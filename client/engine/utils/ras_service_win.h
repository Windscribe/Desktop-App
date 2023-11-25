#pragma once

class IHelper;

// manage SstpSvc and RasMan services (required for IKEv2)
class RAS_Service_win
{
public:
    static RAS_Service_win &instance()
    {
        static RAS_Service_win s;
        return s;
    }

    bool restartRASServices(IHelper *helper);
    bool isRASRunning();

private:
    RAS_Service_win();
};

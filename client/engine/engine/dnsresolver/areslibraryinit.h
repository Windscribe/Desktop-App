#pragma once

class AresLibraryInit
{
public:
    AresLibraryInit();
    ~AresLibraryInit();

    void init();

private:
    bool bInitialized_;
    bool bFailedInitialize_;
};

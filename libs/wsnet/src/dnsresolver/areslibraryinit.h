#pragma once

namespace wsnet {

class AresLibraryInit
{
public:
    AresLibraryInit();
    ~AresLibraryInit();

    bool init();

private:
    bool bInitialized_;
    bool bFailedInitialize_;

    bool initAndroid();
};

} // namespace wsnet

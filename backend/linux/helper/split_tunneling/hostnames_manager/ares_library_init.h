#pragma once

class AresLibraryInit
{
public:
    AresLibraryInit();
    ~AresLibraryInit();

    void init();

private:
    bool init_;
    bool failedInit_;
};


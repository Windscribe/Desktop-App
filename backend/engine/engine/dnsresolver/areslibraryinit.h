#ifndef ARESLIBRARYINIT_H
#define ARESLIBRARYINIT_H


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

#endif // ARESLIBRARYINIT_H

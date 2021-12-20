#ifndef FAKEINSTALLER_H
#define FAKEINSTALLER_H

#include <string>
#include <functional>
#include <Windows.h>

enum INSTALLER_CURRENT_STATE {
    STATE_INIT,
    STATE_EXTRACTING,
    STATE_PAUSED,
    STATE_CANCELED,
    STATE_FINISHED,
    STATE_ERROR,
    STATE_FATAL_ERROR,
};

class FakeInstaller
{
 private:
	std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> callbackState_;
	std::wstring installPath_;
	bool isCreateShortcut_;

	UINT_PTR timerId_;
 public:
    std::wstring getPath();
    void setPath(const std::wstring &path);
    void setCreateShortcut(const bool &create_shortcut);
    bool getCreateShortcut();

    void start();
    void pause();
    void cancel();
    void resume();
    void waitForCompletion();

    void runLauncher();

	FakeInstaller(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState);
    virtual ~FakeInstaller();

	static void CALLBACK timerproc(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4);

};


#endif // FAKEINSTALLER_H

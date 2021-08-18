#include "fakeinstaller.h"
#include <assert.h>


FakeInstaller::FakeInstaller(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState)
{
	assert(callbackState != NULL);
	callbackState_ = callbackState;
	isCreateShortcut_ = true;
	installPath_ = L"C:\\Program Files (x86)\\Windscribe";

}

FakeInstaller::~FakeInstaller()
{
	
}


std::wstring FakeInstaller::getPath()
{
	return installPath_;
}

void FakeInstaller::setPath(const std::wstring &path)
{
	installPath_ = path;
}

void FakeInstaller::setCreateShortcut(const bool &create_shortcut)
{
	isCreateShortcut_ = create_shortcut;
}

bool FakeInstaller::getCreateShortcut()
{
	return isCreateShortcut_;
}

FakeInstaller *f;
int progress = 0;
void FakeInstaller::timerproc(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4)
{
	progress++;

	if (progress >= 77)
	{
		f->callbackState_(progress, STATE_FATAL_ERROR);
		KillTimer(NULL, f->timerId_);
		return;
	}
	if (progress >= 100)
	{
		f->callbackState_(progress, STATE_FINISHED);
		KillTimer(NULL, f->timerId_);
	}
	else
	{
		f->callbackState_(progress, STATE_EXTRACTING);
	}
}

void FakeInstaller::start()
{
	f = this;
	callbackState_(0, STATE_EXTRACTING);
	timerId_ = SetTimer(NULL, 5555, 30, &timerproc);
}

void FakeInstaller::pause()
{
	callbackState_(0, STATE_PAUSED);
	KillTimer(NULL, f->timerId_);
}

void FakeInstaller::resume()
{
	timerId_ = SetTimer(NULL, 5555, 30, &timerproc);
}

void FakeInstaller::cancel()
{

}

void FakeInstaller::waitForCompletion()
{
	
}




void FakeInstaller::runLauncher()
{
 /*#ifdef _WIN32
	wstring pathLauncher = installPath_ + L"\\WindscribeLauncher.exe";
	ShellExecute(nullptr, nullptr, pathLauncher.c_str(), nullptr, nullptr, SW_RESTORE);
 #endif*/
}



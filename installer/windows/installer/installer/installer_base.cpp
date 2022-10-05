#include "installer_base.h"

#ifndef GUI

unsigned InstallerBase::kBreakAbortThreshold=2;
unsigned InstallerBase::g_BreakCounter=0;

#if !defined(UNDER_CE) && defined(_WIN32)
static BOOL WINAPI HandlerRoutine(DWORD ctrlType);
#endif

#if defined __APPLE__
static void HandlerRoutine(int);
#endif

#endif

InstallerBase::InstallerBase(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState)
{
	callbackState_ = callbackState;

	kBreakAbortThreshold = 2;
	g_BreakCounter = 0;


 #ifndef GUI
 #if defined(_WIN32)
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
 #endif

 #if defined __APPLE__
 memo_sig_int = signal(SIGINT,HandlerRoutine); // CTRL-C
 if (memo_sig_int == SIG_ERR)
 throw "SetConsoleCtrlHandler fails (SIGINT)";

 memo_sig_term = signal(SIGTERM,HandlerRoutine); // for kill -15 (before "kill -9")
 if (memo_sig_term == SIG_ERR)
 throw "SetConsoleCtrlHandler fails (SIGTERM)";
 #endif

 #endif

}

InstallerBase::~InstallerBase()
{
	waitForCompletion();

 #ifndef GUI

 #if !defined(UNDER_CE) && defined(_WIN32)
	SetConsoleCtrlHandler(HandlerRoutine, FALSE);
 #endif

 #if defined __APPLE__
 signal(SIGINT,memo_sig_int); // CTRL-C
 signal(SIGTERM,memo_sig_term); // kill {pid}
 #endif

 #endif
}

void InstallerBase::start()
{
	isCanceled_ = false;
	isPaused_ = false;
    startImpl();

	extract_thread = std::thread([this] { executionImpl(); });
}

void InstallerBase::pause()
{
	std::lock_guard<std::mutex> lock(mutex_);
	isPaused_ = true;
}

void InstallerBase::resume()
{
	std::lock_guard<std::mutex> lock(mutex_);
	isPaused_ = false;
}

void InstallerBase::cancel()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		isCanceled_ = true;
	}
	waitForCompletion();

	/*mutex_.lock();
	g_BreakCounter++;
	mutex_.unlock();

	resume();
	waitForCompletion();

 #ifdef _WIN32
	unsigned long ResultCode;

	Process process;
	process.InstExec(installPath_ + L"\\uninstall.exe", L"/VERYSILENT=1", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);
 #endif*/
}

void InstallerBase::waitForCompletion()
{
	if (extract_thread.joinable())
	{
		extract_thread.join();
	}
}

void InstallerBase::runLauncher()
{
    runLauncherImpl();
}

#ifndef GUI
#if !defined(UNDER_CE) && defined(_WIN32)
static BOOL WINAPI HandlerRoutine(DWORD ctrlType)
{
  if (ctrlType == CTRL_LOGOFF_EVENT)
  {
   return TRUE;
  }

  InstallerBase::g_BreakCounter++;

  if (InstallerBase::g_BreakCounter < InstallerBase::kBreakAbortThreshold)
  {
   return TRUE;
  }

  return FALSE;
}
#endif

#if defined __APPLE__
static void HandlerRoutine(int)
{
  InstallerBase::g_BreakCounter++;
  if (InstallerBase::g_BreakCounter < InstallerBase::kBreakAbortThreshold)
    return ;
  exit(EXIT_FAILURE);
}
#endif

#endif



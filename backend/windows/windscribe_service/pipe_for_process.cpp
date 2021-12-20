#include "all_headers.h"
#include "pipe_for_process.h"
#include "../../../common/utils/crashhandler.h"

unsigned long PipeForProcess::pipeSerialNumber_ = 0;

PipeForProcess::PipeForProcess() : hReadPipe_(NULL), hWritePipe_(NULL), hThread_(NULL), hStopEvent_(NULL), is_suspended_output_(false)
{
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	if (!myCreatePipeEx(&hReadPipe_, &hWritePipe_, &sa, PIPE_SIZE, FILE_FLAG_OVERLAPPED, FILE_FLAG_OVERLAPPED))
	{
		hReadPipe_ = NULL;
		hWritePipe_ = NULL;
	}
}

PipeForProcess::~PipeForProcess()
{
	stopThread();
	if (hReadPipe_)
	{
		CloseHandle(hReadPipe_);
	}
	if (hWritePipe_)
	{
		CloseHandle(hWritePipe_);
	}
}

HANDLE PipeForProcess::getPipeHandle()
{
	return hWritePipe_;
}

void PipeForProcess::startReading()
{
	output_.clear();
	is_suspended_output_ = false;
	hStopEvent_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	hThread_ = CreateThread(NULL, 0, readThread, this, 0, NULL);
}

void PipeForProcess::suspendReading()
{
	is_suspended_output_ = true;
	output_.clear();
}

std::string PipeForProcess::stopAndGetOutput()
{
	stopThread();
	return output_;
}

BOOL APIENTRY PipeForProcess::myCreatePipeEx(OUT LPHANDLE lpReadPipe, OUT LPHANDLE lpWritePipe, IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
	IN DWORD nSize, DWORD dwReadMode, DWORD dwWriteMode)
{
	HANDLE readPipeHandle, writePipeHandle;
	wchar_t pipeNameBuffer[MAX_PATH];

	// Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
	if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (nSize == 0)
	{
		nSize = 4096;
	}

	wsprintf(pipeNameBuffer, L"\\\\.\\Pipe\\WindscribeServiceInternalPipe.%08x.%lu",
			 GetCurrentProcessId(), pipeSerialNumber_++);

	readPipeHandle = CreateNamedPipe(pipeNameBuffer,
									  PIPE_ACCESS_INBOUND | dwReadMode,
									  PIPE_TYPE_BYTE | PIPE_WAIT,
									  1,             // Number of pipes
									  nSize,         // Out buffer size
									  nSize,         // In buffer size
									  120 * 1000,    // Timeout in ms
									  lpPipeAttributes);

	if (!readPipeHandle) 
	{
		return FALSE;
	}

	writePipeHandle = CreateFile( pipeNameBuffer, GENERIC_WRITE,
								  0,                         // No sharing
								  lpPipeAttributes,
							      OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL | dwWriteMode,
								  NULL);

	if (INVALID_HANDLE_VALUE == writePipeHandle) 
	{
		DWORD dwError = GetLastError();
		CloseHandle(readPipeHandle);
		SetLastError(dwError);
		return FALSE;
	}

	*lpReadPipe = readPipeHandle;
	*lpWritePipe = writePipeHandle;
	return TRUE;
}

DWORD WINAPI PipeForProcess::readThread(LPVOID lpParam)
{
    BIND_CRASH_HANDLER_FOR_THREAD();
	PipeForProcess *this_ = static_cast<PipeForProcess *>(lpParam);

	HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (hEvent == NULL)
	{
		return 0;
	}
	
	const int BUFFER_SIZE = 4096;
	char buf[BUFFER_SIZE + 1];
	DWORD readword;

	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = hEvent;

	HANDLE hEvents[2];
	hEvents[0] = this_->hStopEvent_;
	hEvents[1] = hEvent;

	while (true)
	{
		::ReadFile(this_->hReadPipe_, buf, BUFFER_SIZE, &readword, &overlapped);

		DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (dwWait == WAIT_OBJECT_0)
		{
			break;
		}
		else if (dwWait == (WAIT_OBJECT_0 + 1))
		{
			GetOverlappedResult(this_->hReadPipe_, &overlapped, &readword, FALSE);
			if (readword != 0 && !this_->is_suspended_output_)
			{
				buf[readword] = '\0';
				std::string cstemp = buf;
				this_->output_ += cstemp;
			}
		}
	}

	CloseHandle(hEvent);
	return 0;
}

void PipeForProcess::stopThread()
{
	if (hThread_)
	{
		SetEvent(hStopEvent_);
		WaitForSingleObject(hThread_, INFINITE);
		CloseHandle(hStopEvent_);
		CloseHandle(hThread_);
		hStopEvent_ = NULL;
		hThread_ = NULL;
	}
}
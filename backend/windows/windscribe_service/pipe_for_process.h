#pragma once

class PipeForProcess
{
public:
	PipeForProcess();
	~PipeForProcess();

	HANDLE getPipeHandle();
	void startReading();
	std::string stopAndGetOutput();

private:
	static unsigned long pipeSerialNumber_;
	const int PIPE_SIZE = 16000;

	HANDLE hReadPipe_;
	HANDLE hWritePipe_;
	HANDLE hThread_;
	HANDLE hStopEvent_;
	std::string output_;

	static BOOL APIENTRY myCreatePipeEx(OUT LPHANDLE lpReadPipe, OUT LPHANDLE lpWritePipe, IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
								 IN DWORD nSize, DWORD dwReadMode, DWORD dwWriteMode);
	static DWORD WINAPI readThread(LPVOID lpParam);
	void stopThread();

};


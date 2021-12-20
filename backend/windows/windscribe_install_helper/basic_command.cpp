#include "basic_command.h"



BasicCommand::BasicCommand(Logger *logger) : logger_(logger)
{
}


BasicCommand::~BasicCommand()
{
	delete logger_;
}

bool BasicCommand::executeBlockingCommand(const wchar_t *cmd, std::string &answer)
{
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	HANDLE rPipe, wPipe;
	if (!CreatePipe(&rPipe, &wPipe, &sa, 0))
	{
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdInput = NULL;
	si.hStdError = wPipe;
	si.hStdOutput = wPipe;

	ZeroMemory(&pi, sizeof(pi));
	if (CreateProcess(NULL, (LPWSTR)cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);

		CloseHandle(wPipe);
		answer = readAllFromPipe(rPipe);
		CloseHandle(rPipe);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return true;
	}
	else
	{
		CloseHandle(rPipe);
		CloseHandle(wPipe);
		return false;
	}
}

std::string BasicCommand::readAllFromPipe(HANDLE hPipe)
{
	const int BUFFER_SIZE = 4096;
	std::string csoutput;
	while (true)
	{
		char buf[BUFFER_SIZE + 1];
		DWORD readword;
		if (!::ReadFile(hPipe, buf, BUFFER_SIZE, &readword, 0))
		{
			break;
		}
		if (readword == 0)
		{
			break;
		}
		buf[readword] = '\0';
		std::string cstemp = buf;
		csoutput += cstemp;
	}
	return csoutput;

}

#include "ioutils.h"
#include "logger.h"

bool IOUtils::readAll(HANDLE hPipe, HANDLE hIOEvent, char *buf, DWORD len)
{
    OVERLAPPED overlapped;
    char* dataBuf = buf;
    DWORD bytesToRead = len;

    while (bytesToRead > 0) {
        ::ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = hIOEvent;

        DWORD bytesRead = 0;
        auto result = ::ReadFile(hPipe, dataBuf, bytesToRead, &bytesRead, &overlapped);
        if (result) {
            // The asynchronous read request could be satisfied immediately.
            bytesToRead -= bytesRead;
            dataBuf += bytesRead;
            continue;
        }

        auto lastError = ::GetLastError();
        if (lastError != ERROR_IO_PENDING) {
            Logger::instance().out(L"IOUtils::readAll ReadFile failed %lu", lastError);
            return false;
        }

        result = ::GetOverlappedResultEx(hPipe, &overlapped, &bytesRead, INFINITE, TRUE);
        if (result) {
            bytesToRead -= bytesRead;
            dataBuf += bytesRead;
            continue;
        }

        // An APC is queued to this thread to instruct it to cancel its IO.  No need to log a
        // failure in that case, or if the pipe has been closed by the client.
        lastError = ::GetLastError();
        if (lastError != WAIT_IO_COMPLETION && lastError != ERROR_BROKEN_PIPE) {
            Logger::instance().out(L"IOUtils::readAll GetOverlappedResultEx failed %lu", lastError);
        }

        return false;
    }

    return true;
}

bool IOUtils::writeAll(HANDLE hPipe, HANDLE hIOEvent, const char *buf, DWORD len)
{
    OVERLAPPED overlapped;
    const char* dataBuf = buf;
    DWORD bytesToWrite = len;

    while (bytesToWrite > 0) {
        ::ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = hIOEvent;

        DWORD bytesWritten = 0;
        auto result = ::WriteFile(hPipe, dataBuf, bytesToWrite, &bytesWritten, &overlapped);
        if (result) {
            // The asynchronous write request could be satisfied immediately.
            bytesToWrite -= bytesWritten;
            dataBuf += bytesWritten;
            continue;
        }

        auto lastError = ::GetLastError();
        if (lastError != ERROR_IO_PENDING) {
            Logger::instance().out(L"IOUtils::writeAll WriteFile failed %lu", lastError);
            return false;
        }

        result = ::GetOverlappedResultEx(hPipe, &overlapped, &bytesWritten, INFINITE, TRUE);
        if (result) {
            bytesToWrite -= bytesWritten;
            dataBuf += bytesWritten;
            continue;
        }

        // An APC is queued to this thread to instruct it to cancel its IO.  No need to log a
        // failure in that case, or if the pipe has been closed by the client.
        lastError = ::GetLastError();
        if (lastError != WAIT_IO_COMPLETION && lastError != ERROR_BROKEN_PIPE) {
            Logger::instance().out(L"IOUtils::writeAll GetOverlappedResultEx failed %lu", lastError);
        }

        return false;
    }

    return true;
}

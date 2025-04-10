#include "helperbackend_win.h"
#include <QCoreApplication>
#include <QMutexLocker>
#include "utils/executable_signature/executable_signature.h"
#include "utils/log/categories.h"
#include "installhelper_win.h"


#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\WindscribeService")

//  the program to connect to the helper socket without starting the service (uncomment for debug purpose)
//  #define DEBUG_DONT_USE_SERVICE

HelperBackend_win::HelperBackend_win(QObject *parent) : IHelperBackend(parent)
{
    initVariables();
}

HelperBackend_win::~HelperBackend_win()
{
    scm_.blockStartStopRequests();
    wait();
}

void HelperBackend_win::startInstallHelper()
{
    QMutexLocker locker(&mutex_);
    initVariables();

    QString serviceExePath = QCoreApplication::applicationDirPath() + "/WindscribeService.exe";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(serviceExePath.toStdWString())) {
        qCCritical(LOG_BASIC) << "WindscribeService signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        curState_ = State::kUserCanceled;
        return;
    }

    start(QThread::LowPriority);
}

HelperBackend_win::State HelperBackend_win::currentState() const
{
    QMutexLocker locker(&mutex_);
    return curState_;
}

bool HelperBackend_win::reinstallHelper()
{
    QMutexLocker locker(&mutex_);
    return InstallHelper_win::executeInstallHelperCmd();
}

void HelperBackend_win::run()
{
    QMutexLocker locker(&mutex_);
#ifndef DEBUG_DONT_USE_SERVICE
    try {
        scm_.openSCM(SC_MANAGER_CONNECT);
        scm_.openService(L"WindscribeService", SERVICE_QUERY_STATUS | SERVICE_START);
        auto status = scm_.queryServiceStatus();
        if (status != SERVICE_RUNNING) {
            scm_.startService();
        }

        // The SCM reports the service as running.  Verify we can connect to the helper pipe.
        if (connectToHelper()) {
            curState_ = State::kConnected;
        } else {
            curState_ = State::kFailedConnect;
        }
    }
    catch (std::system_error& ex) {
        qCCritical(LOG_BASIC) << "Helper_win::run -" << ex.what();
        curState_ = State::kFailedConnect;
    }

    scm_.closeSCM();
#else
    curState_ = State::kConnected;
#endif
}

std::string HelperBackend_win::sendCmd(int cmdId, const std::string &data)
{
    QMutexLocker locker(&mutex_);

    if (!connectToHelper()) {
        emit lostConnectionToHelper();
        return std::string();
    }

    try {
        // first 4 bytes - cmdId
        if (!writeAllToPipe(helperPipe_.getHandle(), (char *)&cmdId, sizeof(cmdId))) {
            throw 1;
        }

        // second 4 bytes - size of buffer
        unsigned long sizeOfBuf = data.size();
        if (!writeAllToPipe(helperPipe_.getHandle(), (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
            throw 1;
        }

        // body of message
        if (sizeOfBuf > 0) {
            if (!writeAllToPipe(helperPipe_.getHandle(), data.c_str(), sizeOfBuf)) {
                throw 1;
            }
        }
    } catch (...) {
        qCCritical(LOG_BASIC) << "HelperBackend_win::sendCmd, writeAllToPipe() call failed";
        helperPipe_.closeHandle();
        curState_ = State::kFailedConnect;
        emit lostConnectionToHelper();
        return std::string();
    }

    // read answer
    try {
        unsigned long sizeOfBuf;
        if (!readAllFromPipe(helperPipe_.getHandle(), (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
            throw 1;
        }

        if (sizeOfBuf > 0) {
            QScopedArrayPointer<char> buf(new char[sizeOfBuf]);
            if (!readAllFromPipe(helperPipe_.getHandle(), buf.data(), sizeOfBuf)) {
                throw 1;
            }

            return std::string(buf.data(), sizeOfBuf);
        }
    } catch (...) {
        qCCritical(LOG_BASIC) << "HelperBackend_win::sendCmd, readAllFromPipe() call failed";
        helperPipe_.closeHandle();
        curState_ = State::kFailedConnect;
        emit lostConnectionToHelper();
        return std::string();
    }
    return std::string();
}

void HelperBackend_win::initVariables()
{
    curState_= State::kInit;
    scm_.unblockStartStopRequests();
}

bool HelperBackend_win::readAllFromPipe(HANDLE hPipe, char *buf, DWORD len)
{
    bool result = true;
    char *ptr = buf;
    DWORD dwRead = 0;
    DWORD lenCopy = len;
    while (lenCopy > 0) {
        if (::ReadFile(hPipe, ptr, lenCopy, &dwRead, 0) == FALSE) {
            qCWarning(LOG_IPC) << "Failed to read from the helper pipe:" << ::GetLastError();
            result = false;
            break;
        }
        ptr += dwRead;
        lenCopy -= dwRead;
    }
    return true;
}

bool HelperBackend_win::writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len)
{
    bool result = true;
    const char *ptr = buf;
    DWORD dwWrite = 0;
    while (len > 0) {
        if (::WriteFile(hPipe, ptr, len, &dwWrite, 0) == FALSE) {
            qCWarning(LOG_IPC) << "Failed to write to the helper pipe:" << ::GetLastError();
            result = false;
            break;
        }
        ptr += dwWrite;
        len -= dwWrite;
    }
    return true;
}

bool HelperBackend_win::connectToHelper()
{
    if (helperPipe_.isValid()) {
        // Check if our IPC connection has become invalid (e.g. the helper is restarted while the app is running).
        DWORD flags;
        BOOL result = ::GetNamedPipeInfo(helperPipe_.getHandle(), &flags, NULL, NULL, NULL);
        if (result == FALSE) {
            qCCritical(LOG_IPC) << "Reconnecting helper pipe as existing connection is invalid:" << ::GetLastError();
            helperPipe_.closeHandle();
        }
    }

    if (!helperPipe_.isValid()) {
        HANDLE hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (hPipe == INVALID_HANDLE_VALUE) {
            if (::WaitNamedPipe(SERVICE_PIPE_NAME, MAX_WAIT_TIME_FOR_PIPE) == FALSE) {
                qCCritical(LOG_IPC) << "Wait for helper pipe to become available failed:" << ::GetLastError();
                return false;
            }

            hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
            if (hPipe == INVALID_HANDLE_VALUE) {
                qCCritical(LOG_IPC) << "Failed to connect to the helper pipe:" << ::GetLastError();
                return false;
            }
        }

        helperPipe_.setHandle(hPipe);
    }

    return true;
}

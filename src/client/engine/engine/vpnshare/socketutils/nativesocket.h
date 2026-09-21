#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <winsock2.h>
#else
#include <unistd.h>
#endif

namespace SocketUtils {

// For accepted descriptors that no QTcpSocket adopted: Qt leaves those open and nothing else will release them.
inline void closeNativeSocket(qintptr socketDescriptor)
{
#ifdef Q_OS_WIN
    ::closesocket(static_cast<SOCKET>(socketDescriptor));
#else
    ::close(static_cast<int>(socketDescriptor));
#endif
}

} // namespace SocketUtils

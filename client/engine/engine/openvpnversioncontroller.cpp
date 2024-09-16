#include "openvpnversioncontroller.h"

#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include "utils/winutils.h"
#else
#include "boost/process.hpp"
#include <QRegExp>
#endif

#include "utils/ws_assert.h"

QString OpenVpnVersionController::getOpenVpnVersion()
{
    if (ovpnVersion_.isEmpty()) {
        detectVersion();
    }

    return ovpnVersion_;
}

QString OpenVpnVersionController::getOpenVpnFilePath()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    QString exe = QCoreApplication::applicationDirPath() + QDir::separator() + getOpenVpnFileName();
#elif defined Q_OS_MACOS
    QString exe = QCoreApplication::applicationDirPath() + "/../Helpers/" + getOpenVpnFileName();
#else
#error OpenVpnVersionController::getOpenVpnFilePath() not implemented for this platform.
#endif
    return exe;
}

QString OpenVpnVersionController::getOpenVpnFileName()
{
#ifdef Q_OS_WIN
    return QString("windscribeopenvpn.exe");
#else
    return QString("windscribeopenvpn");
#endif
}

OpenVpnVersionController::OpenVpnVersionController()
{
}

void OpenVpnVersionController::detectVersion()
{
    ovpnVersion_.clear();

    QString exe = getOpenVpnFilePath();

    if (!QFile::exists(exe)) {
        return;
    }

#ifdef Q_OS_WIN
    ovpnVersion_ = WinUtils::getVersionInfoItem(exe, "ProductVersion");

    // Trim fourth component of version number (e.g. 2.6.1.0 -> 2.6.1) if necessary.
    if (ovpnVersion_.count('.') > 2) {
        QStringList parts = ovpnVersion_.split('.');
        WS_ASSERT(parts.size() >= 3);
        ovpnVersion_ = QString("%1.%2.%3").arg(parts.at(0), parts.at(1), parts.at(2));
    }
#else

    using namespace boost::process;
    ipstream pipe_stream;
    child c(exe.toStdString(), "--version", std_out > pipe_stream);

    std::string line;
    QString strAnswer;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        strAnswer += QString::fromStdString(line).toLower();

    c.wait();

    // parse version from process output
    QRegExp rx("\\d{1,}.\\d{1,}.\\d{1,}");
    rx.indexIn(strAnswer);
    QStringList list = rx.capturedTexts();
    WS_ASSERT(list.count() == 1);
    if (list.count() == 1) {
        ovpnVersion_ = list[0];
    }
#endif
}

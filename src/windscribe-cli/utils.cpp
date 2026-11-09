#include "utils.h"

#include <QString>
#include <QStringList>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef Q_OS_LINUX
    #include <termios.h>
    #include <unistd.h>
#elif defined(Q_OS_MACOS)
    #include <readpassphrase.h>
    #include <unistd.h>
#else
    #include <io.h>
    #include <windows.h>
#endif

#if defined(Q_OS_WIN)
// mintty (Git Bash, MSYS2, Cygwin) gives a program a pipe instead of a console, named
// \\{cygwin,msys}-<hash>-pty<N>-{from,to}-master.
static bool isCygwinPty(HANDLE handle)
{
    if (GetFileType(handle) != FILE_TYPE_PIPE) {
        return false;
    }

    struct {
        FILE_NAME_INFO info;
        WCHAR name[MAX_PATH];
    } buf = {};
    if (!GetFileInformationByHandleEx(handle, FileNameInfo, &buf, sizeof(buf))) {
        return false;
    }

    const QString name = QString::fromWCharArray(buf.info.FileName, buf.info.FileNameLength / sizeof(WCHAR));
    const QStringList parts = name.split('-');
    return parts.size() >= 5 && (parts[0] == "\\cygwin" || parts[0] == "\\msys") && parts[2].startsWith("pty") &&
           (parts[3] == "from" || parts[3] == "to") && parts[4] == "master";
}
#endif

bool Utils::isInteractive()
{
#if defined(Q_OS_WIN)
    return _isatty(_fileno(stdin)) != 0 || isCygwinPty(GetStdHandle(STD_INPUT_HANDLE));
#else
    return isatty(STDIN_FILENO);
#endif
}

QString Utils::readLine()
{
    std::string input;
    if (!std::getline(std::cin, input)) {
        return QString();
    }
    if (!input.empty() && input.back() == '\r') {
        input.pop_back();
    }
    return QString::fromStdString(input);
}

bool Utils::canDisableEcho()
{
#if defined(Q_OS_WIN)
    DWORD mode;
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
#else
    return true;
#endif
}

QString Utils::getInput(const QString &prompt, bool disableEcho)
{
#if defined(Q_OS_WIN)
    DWORD mode = 0;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    const bool isEchoDisabled = disableEcho && GetConsoleMode(hStdin, &mode) &&
                                SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);

    std::string input;
    std::cout << prompt.toStdString();
    std::cin >> input;

    if (isEchoDisabled) {
        std::cout << std::endl;
        SetConsoleMode(hStdin, mode);
    }

    return QString::fromStdString(input);
#elif defined(Q_OS_MAC)
    char buf[1024];
    memset(buf, 0, 1024);

    char *ret = readpassphrase(prompt.toStdString().c_str(), buf, 1024, disableEcho ? RPP_ECHO_OFF : RPP_ECHO_ON);
    if (ret == NULL) {
        return "";
    }
    return ret;
#else
    struct termios tty;
    tcflag_t oldFlags;

    if (disableEcho) {
        tcgetattr(STDIN_FILENO, &tty);
        oldFlags = tty.c_lflag;
        tty.c_lflag &= ~ECHO;
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }

    std::cout << prompt.toStdString();
    std::string input;
    std::cin >> input;

    if (disableEcho) {
        std::cout << std::endl;
        tty.c_lflag = oldFlags;
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }

    return QString::fromStdString(input);
#endif
}

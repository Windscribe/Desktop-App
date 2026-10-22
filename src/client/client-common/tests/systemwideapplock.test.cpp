#include "systemwideapplock.test.h"

#include <QtTest>

#include "utils/systemwideapplock.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using Holder = SystemWideAppLock::Holder;

namespace {

int toInt(Holder holder)
{
    return static_cast<int>(holder);
}

#if !defined(Q_OS_WIN)

// A record lock belongs to a process, so the other holder has to be a separate process.
struct HolderProcess
{
    pid_t pid = -1;
    int toChild = -1;
    Holder acquired = Holder::kUnknown;

    explicit HolderProcess(const std::string &name)
    {
        int down[2];
        int up[2];
        if (pipe(down) != 0 || pipe(up) != 0) {
            return;
        }
        pid = fork();
        if (pid == 0) {
            close(down[1]);
            close(up[0]);
            SystemWideAppLock lock(name);
            const char result = static_cast<char>(lock.acquire());
            (void)write(up[1], &result, 1);
            char c;
            (void)read(down[0], &c, 1);
            _exit(0);
        }
        close(down[0]);
        close(up[1]);
        toChild = down[1];
        char result = static_cast<char>(Holder::kUnknown);
        (void)read(up[0], &result, 1);
        close(up[0]);
        acquired = static_cast<Holder>(result);
    }

    void kill()
    {
        ::kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        pid = -1;
    }

    ~HolderProcess()
    {
        if (pid > 0) {
            const char c = 0;
            (void)write(toChild, &c, 1);
            waitpid(pid, nullptr, 0);
        }
        if (toChild >= 0) {
            close(toChild);
        }
    }
};

Holder holderSeenFromAnotherProcess(const std::string &name)
{
    const pid_t pid = fork();
    if (pid == 0) {
        _exit(static_cast<int>(SystemWideAppLock::holder(name)));
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return static_cast<Holder>(WEXITSTATUS(status));
}

#endif

} // namespace

void TestSystemWideAppLock::init()
{
#if defined(Q_OS_WIN)
    name_ = "Local\\WindscribeSystemWideAppLockTest" + std::to_string(GetCurrentProcessId());
#else
    QVERIFY(dir_.isValid());
    name_ = dir_.filePath("app.lock").toStdString();
    unlink(name_.c_str());
    const int fd = open(name_.c_str(), O_CREAT | O_RDWR, 0666);
    QVERIFY(fd >= 0);
    close(fd);
    QCOMPARE(chmod(name_.c_str(), 0666), 0);
#endif
}

void TestSystemWideAppLock::testFreeLock()
{
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kNone));
}

void TestSystemWideAppLock::testAcquire()
{
    SystemWideAppLock lock(name_);
    QCOMPARE(toInt(lock.acquire()), toInt(Holder::kNone));
#if defined(Q_OS_WIN)
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kThisUser));
#else
    QCOMPARE(toInt(holderSeenFromAnotherProcess(name_)), toInt(Holder::kThisUser));
#endif
}

void TestSystemWideAppLock::testHeldByAnotherProcess()
{
#if defined(Q_OS_WIN)
    SystemWideAppLock first(name_);
    QCOMPARE(toInt(first.acquire()), toInt(Holder::kNone));
    SystemWideAppLock second(name_);
    QCOMPARE(toInt(second.acquire()), toInt(Holder::kThisUser));
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kThisUser));
#else
    HolderProcess holder(name_);
    QCOMPARE(toInt(holder.acquired), toInt(Holder::kNone));
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kThisUser));

    SystemWideAppLock second(name_);
    QCOMPARE(toInt(second.acquire()), toInt(Holder::kThisUser));

    // Neither the refused attempt nor the queries may loosen the holder's lock.
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kThisUser));
#endif
}

void TestSystemWideAppLock::testReleasedWhenHolderExits()
{
#if defined(Q_OS_WIN)
    {
        SystemWideAppLock lock(name_);
        QCOMPARE(toInt(lock.acquire()), toInt(Holder::kNone));
    }
#else
    HolderProcess holder(name_);
    QCOMPARE(toInt(holder.acquired), toInt(Holder::kNone));
    holder.kill();
#endif
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kNone));
    SystemWideAppLock lock(name_);
    QCOMPARE(toInt(lock.acquire()), toInt(Holder::kNone));
}

#if !defined(Q_OS_WIN)

void TestSystemWideAppLock::testMissingFile()
{
    const std::string missing = dir_.filePath("missing.lock").toStdString();
    QCOMPARE(toInt(SystemWideAppLock::holder(missing)), toInt(Holder::kUnknown));
    SystemWideAppLock lock(missing);
    QCOMPARE(toInt(lock.acquire()), toInt(Holder::kUnknown));
}

void TestSystemWideAppLock::testReadOnlyFile()
{
    if (geteuid() == 0) {
        QSKIP("root can write to a read-only file");
    }
    QCOMPARE(chmod(name_.c_str(), 0444), 0);
    SystemWideAppLock lock(name_);
    QCOMPARE(toInt(lock.acquire()), toInt(Holder::kUnknown));
    QCOMPARE(toInt(SystemWideAppLock::holder(name_)), toInt(Holder::kNone));
}

#endif

QTEST_APPLESS_MAIN(TestSystemWideAppLock)

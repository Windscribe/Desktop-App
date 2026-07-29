#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "networkutilslinux.test.h"
#include "utils/network_utils/network_utils_linux.h"

namespace {

// Builds a fake /proc: if_inet6 present unless stackPresent is false, and the two disable_ipv6
// sysctls written only when a value is supplied.
class FakeProc
{
public:
    FakeProc(bool stackPresent, const char *all, const char *dflt)
    {
        QVERIFY2(dir_.isValid(), "could not create the fake /proc");
        QDir(dir_.path()).mkpath("net");
        QDir(dir_.path()).mkpath("sys/net/ipv6/conf/all");
        QDir(dir_.path()).mkpath("sys/net/ipv6/conf/default");

        if (stackPresent) {
            write("net/if_inet6", "");
        }
        if (all) {
            write("sys/net/ipv6/conf/all/disable_ipv6", all);
        }
        if (dflt) {
            write("sys/net/ipv6/conf/default/disable_ipv6", dflt);
        }
    }

    QString path() const { return dir_.path(); }

private:
    void write(const QString &relative, const QByteArray &contents)
    {
        QFile f(dir_.path() + "/" + relative);
        QVERIFY2(f.open(QFile::WriteOnly), qPrintable(f.fileName()));
        f.write(contents);
    }

    QTemporaryDir dir_;
};

}

void TestNetworkUtilsLinux::testIsIpv6EnabledWhenStackAbsent()
{
    // ipv6.disable=1 on the kernel command line: no if_inet6 and no sysctls at all.
    FakeProc proc(false, nullptr, nullptr);
    QVERIFY(!NetworkUtils_linux::isIpv6Enabled(proc.path()));
}

void TestNetworkUtilsLinux::testIsIpv6EnabledWhenDisabledForAll()
{
    FakeProc proc(true, "1\n", "1\n");
    QVERIFY(!NetworkUtils_linux::isIpv6Enabled(proc.path()));

    // A write to `all` propagates into `default`, but tolerate the two disagreeing.
    FakeProc allOnly(true, "1\n", "0\n");
    QVERIFY(!NetworkUtils_linux::isIpv6Enabled(allOnly.path()));

    // Any nonzero disables, and a raw write leaves no trailing newline.
    FakeProc nonOne(true, "2", "2");
    QVERIFY(!NetworkUtils_linux::isIpv6Enabled(nonOne.path()));
}

void TestNetworkUtilsLinux::testIsIpv6EnabledWhenDisabledForDefault()
{
    // `default` is what a freshly created wg device inherits, so it alone is decisive.
    FakeProc proc(true, "0\n", "1\n");
    QVERIFY(!NetworkUtils_linux::isIpv6Enabled(proc.path()));
}

void TestNetworkUtilsLinux::testIsIpv6EnabledWhenEnabled()
{
    FakeProc proc(true, "0\n", "0\n");
    QVERIFY(NetworkUtils_linux::isIpv6Enabled(proc.path()));
}

void TestNetworkUtilsLinux::testIsIpv6EnabledWhenSysctlsUnreadable()
{
    // Stack present but the sysctls are missing: assume enabled rather than downgrading tunnels.
    FakeProc proc(true, nullptr, nullptr);
    QVERIFY(NetworkUtils_linux::isIpv6Enabled(proc.path()));
}

QTEST_MAIN(TestNetworkUtilsLinux)

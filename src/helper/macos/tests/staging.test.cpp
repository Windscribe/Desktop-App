// Unit tests for Staging::freezeRegularFile — the safe single-file copy used to freeze the downloaded
// DMG before verification. Pins that it copies a regular file's bytes at mode 0600 and refuses every
// smuggle vector (symlink source, non-regular source, pre-existing destination). No test framework;
// returns the number of failed checks (0 on success). Runs as the test user; no root or DMG needed.

#include <cstdio>
#include <filesystem>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../installer/staging.h"

namespace
{

int g_failures = 0;

bool check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
    return ok;
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

std::string g_dir;

std::string p(const std::string &name)
{
    return g_dir + "/" + name;
}

void writeFile(const std::string &path, const std::string &contents)
{
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        (void)write(fd, contents.data(), contents.size());
        close(fd);
    }
}

std::string readFile(const std::string &path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return {};
    }
    std::string out;
    char buf[512];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        out.append(buf, static_cast<size_t>(n));
    }
    close(fd);
    return out;
}

bool exists(const std::string &path)
{
    struct stat st;
    return lstat(path.c_str(), &st) == 0;
}

void testCopiesRegularFile()
{
    writeFile(p("src.dmg"), "disk image bytes");
    VERIFY(Staging::freezeRegularFile(p("src.dmg"), p("dst.dmg")));
    VERIFY(readFile(p("dst.dmg")) == "disk image bytes");
    struct stat st;
    VERIFY(lstat(p("dst.dmg").c_str(), &st) == 0);
    VERIFY((st.st_mode & 07777) == (S_IRUSR | S_IWUSR));
}

void testRejectsSymlinkSource()
{
    writeFile(p("target"), "root-only-secret");
    VERIFY(symlink(p("target").c_str(), p("link").c_str()) == 0);
    VERIFY(!Staging::freezeRegularFile(p("link"), p("out_symlink")));
    VERIFY(!exists(p("out_symlink")));
}

void testRejectsFifoSource()
{
    VERIFY(mkfifo(p("fifo").c_str(), 0644) == 0);
    VERIFY(!Staging::freezeRegularFile(p("fifo"), p("out_fifo")));
    VERIFY(!exists(p("out_fifo")));
}

void testRejectsDirectorySource()
{
    VERIFY(mkdir(p("adir").c_str(), 0755) == 0);
    VERIFY(!Staging::freezeRegularFile(p("adir"), p("out_dir")));
    VERIFY(!exists(p("out_dir")));
}

void testRejectsExistingDst()
{
    writeFile(p("src2.dmg"), "new");
    writeFile(p("existing"), "old");
    VERIFY(!Staging::freezeRegularFile(p("src2.dmg"), p("existing")));
    VERIFY(readFile(p("existing")) == "old");
}

void testRejectsMissingSource()
{
    VERIFY(!Staging::freezeRegularFile(p("does_not_exist"), p("out_missing")));
    VERIFY(!exists(p("out_missing")));
}

void testRunWithTimeoutSuccess()
{
    VERIFY(Staging::runWithTimeout("/usr/bin/true", {}, 5) == 0);
}

void testRunWithTimeoutNonzeroExit()
{
    VERIFY(Staging::runWithTimeout("/usr/bin/false", {}, 5) == 1);
}

void testRunWithTimeoutTimesOut()
{
    // sleep 5 under a 1s bound must be killed and report timeout (-2), not run to completion.
    VERIFY(Staging::runWithTimeout("/bin/sleep", {"5"}, 1) == -2);
}

void testRunWithTimeoutSpawnFailure()
{
    VERIFY(Staging::runWithTimeout("/nonexistent/binary/xyzzy", {}, 5) != 0);
}

} // namespace

int main()
{
    char tmpl[] = "/tmp/staging.test.XXXXXX";
    const char *dir = mkdtemp(tmpl);
    if (!dir) {
        printf("FAIL: could not create temp dir\n");
        return 1;
    }
    g_dir = dir;

    testCopiesRegularFile();
    testRejectsSymlinkSource();
    testRejectsFifoSource();
    testRejectsDirectorySource();
    testRejectsExistingDst();
    testRejectsMissingSource();
    testRunWithTimeoutSuccess();
    testRunWithTimeoutNonzeroExit();
    testRunWithTimeoutTimesOut();
    testRunWithTimeoutSpawnFailure();

    std::error_code cleanupEc;
    std::filesystem::remove_all(g_dir, cleanupEc);

    if (g_failures == 0) {
        printf("All staging tests passed\n");
    }
    return g_failures;
}

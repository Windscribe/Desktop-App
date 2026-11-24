#include <cstdio>
#include <cstdlib>
#include <string>

#include "../spawn_posix.h"

namespace {

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

void testExitStatusIsTheProgramsOwn()
{
    std::string out;
    VERIFY(Spawn::runCommand("sh", {"-c", "exit 3"}, &out, true) == 3);
    VERIFY(Spawn::runCommand("true", {}, nullptr, true) == 0);
}

void testArgumentsReachTheProgramVerbatim()
{
    std::string out;
    VERIFY(Spawn::runCommand("printf", {"%s", "a b;$HOME*'\""}, &out, true) == 0);
    VERIFY(out == "a b;$HOME*'\"\n");
}

void testOutputEndsWithANewline()
{
    std::string out;
    VERIFY(Spawn::runCommand("printf", {"a\nb"}, &out, true) == 0);
    VERIFY(out == "a\nb\n");
    VERIFY(Spawn::runCommand("printf", {"a\n"}, &out, true) == 0);
    VERIFY(out == "a\n");
    VERIFY(Spawn::runCommand("true", {}, &out, true) == 0);
    VERIFY(out.empty());
}

void testStderrIsMergedOrDiscarded()
{
    std::string out;
    VERIFY(Spawn::runCommand("sh", {"-c", "echo out; echo err >&2"}, &out, true) == 0);
    VERIFY(out == "out\nerr\n");
    VERIFY(Spawn::runCommand("sh", {"-c", "echo out; echo err >&2"}, &out, false) == 0);
    VERIFY(out == "out\n");
}

void testMissingProgram()
{
    std::string out;
    VERIFY(Spawn::runCommand("wsruncommandtest-no-such-program", {}, &out, true) == 127);
    VERIFY(out.find("wsruncommandtest-no-such-program") != std::string::npos);
    VERIFY(Spawn::runCommand("wsruncommandtest-no-such-program", {}, &out, false) == 127);
    VERIFY(out.empty());
}

void testEnvironmentOverridesAndRemovals()
{
    setenv("WS_RUN_COMMAND_A", "parent", 1);
    setenv("WS_RUN_COMMAND_B", "parent", 1);
    std::string out;
    VERIFY(Spawn::runCommand("sh", {"-c", "echo \"$WS_RUN_COMMAND_A/${WS_RUN_COMMAND_B-unset}\""}, &out, true,
                             {{"WS_RUN_COMMAND_A", "child"}}, {"WS_RUN_COMMAND_B"}) == 0);
    VERIFY(out == "child/unset\n");
    VERIFY(Spawn::runCommand("sh", {"-c", "echo \"$WS_RUN_COMMAND_A/${WS_RUN_COMMAND_B-unset}\""}, &out, true) == 0);
    VERIFY(out == "parent/parent\n");
}

void testKilledProgramIsNotSuccess()
{
    VERIFY(Spawn::runCommand("sh", {"-c", "kill -9 $$"}, nullptr, true) == -1);
}

} // namespace

int main()
{
    testExitStatusIsTheProgramsOwn();
    testArgumentsReachTheProgramVerbatim();
    testOutputEndsWithANewline();
    testStderrIsMergedOrDiscarded();
    testMissingProgram();
    testEnvironmentOverridesAndRemovals();
    testKilledProgramIsNotSuccess();
    if (g_failures != 0) {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}

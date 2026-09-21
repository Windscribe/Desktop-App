// Pins the environment Utils::executeCommand hands to /bin/sh: system utilities must produce untranslated
// output for the callers that parse it, whatever locale the helper itself inherited.

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../utils.h"

namespace
{

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        fprintf(stderr, "FAILED line %d: %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

// The child must see the pinned locale even when the test process itself carries a translated one.
void inheritTranslatedLocale()
{
    setenv("LC_ALL", "de_DE.UTF-8", 1);
    setenv("LANGUAGE", "de_DE:de", 1);
}

const char *const kProbe = "echo \"$LC_ALL/${LANGUAGE-unset}\"";

void testArgvFormPinsLocale()
{
    inheritTranslatedLocale();
    std::string out;
    const int rc = Utils::executeCommand("sh", {"-c", kProbe}, &out);
    VERIFY(rc == 0);
    VERIFY(out == "C.UTF-8/unset\n");
}

void testPipelineFormPinsLocaleInEveryStage()
{
    inheritTranslatedLocale();
    std::string out;
    const int rc = Utils::executeCommand(std::string("true | sh -c '") + kProbe + "'", {}, &out, false);
    VERIFY(rc == 0);
    VERIFY(out == "C.UTF-8/unset\n");
}

void testExitStatusIsTheCommandsOwn()
{
    std::string out;
    VERIFY(Utils::executeCommand("sh", {"-c", "exit 3"}, &out) == 3);
}

} // namespace

int main()
{
    testArgvFormPinsLocale();
    testPipelineFormPinsLocaleInEveryStage();
    testExitStatusIsTheCommandsOwn();

    if (g_failures == 0) {
        printf("All helper utils tests passed\n");
    }
    return g_failures;
}

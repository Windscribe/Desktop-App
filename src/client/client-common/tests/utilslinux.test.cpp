#include "utilslinux.test.h"

#include <QtTest>

#include "utils/utils.h"

namespace {

// The child must see the pinned locale even when the test process itself carries a translated one.
void inheritTranslatedLocale()
{
    qputenv("LC_ALL", "de_DE.UTF-8");
    qputenv("LANGUAGE", "de_DE:de");
}

const char *const kProbe = "echo \"$LC_ALL/${LANGUAGE-unset}\"";

} // namespace

void TestUtilsLinux::testExecCmdPinsLocale()
{
    inheritTranslatedLocale();
    QCOMPARE(Utils::execCmd(kProbe), QString("C.UTF-8/unset\n"));
}

void TestUtilsLinux::testExecCmdPinsLocaleInEveryPipelineStage()
{
    inheritTranslatedLocale();
    QCOMPARE(Utils::execCmd(QString("true | sh -c '") + kProbe + "'"), QString("C.UTF-8/unset\n"));
}

QTEST_MAIN(TestUtilsLinux)

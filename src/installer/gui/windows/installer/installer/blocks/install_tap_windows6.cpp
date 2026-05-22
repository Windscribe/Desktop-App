#include "install_tap_windows6.h"

#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <spdlog/spdlog.h>

#include "installerenums.h"
#include "../installer_utils.h"
#include "../settings.h"
#include "../../../utils/applicationinfo.h"

using namespace std;

InstallTapWindows6::InstallTapWindows6(double weight) : IInstallBlock(weight, L"tapwindows6", false)
{
}

int InstallTapWindows6::executeStep()
{
    const QString installPath = QString::fromStdWString(Settings::instance().getPath());
    const QString program = QString("\"%1\\devcon.exe\"").arg(installPath);
    const QString infFile = "tapwindows6\\OemVista.inf";

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setWorkingDirectory(installPath);
    process.start(program, QStringList() << "dp_add" << infFile);
    process.waitForFinished();
    QByteArray appOutput = process.readAll();

    if (process.exitCode() != 0) {
        spdlog::error(L"InstallTapWindows6: devcon.exe returned exit code {}", process.exitCode());
        spdlog::info("InstallTapWindows6: devcon.exe output ({})", appOutput.constData());
        return -wsl::ERROR_OTHER;
    }

    // Parse the OEM identifier from the output and store it for use during uninstall.
    QRegularExpression re("oem\\d+.inf");
    const QRegularExpressionMatch match = re.match(appOutput);
    if (!match.hasMatch()) {
        spdlog::error("InstallTapWindows6: failed to find OEM identifier in devcon.exe output ({})", appOutput.constData());
        return -wsl::ERROR_OTHER;
    }

    QString adapterOEMIdentifier = match.captured(0);
    QSettings reg(QString::fromStdWString(ApplicationInfo::installerRegistryKey()), QSettings::NativeFormat);
    reg.setValue("tapWindows6DriverOEMIdentifier", adapterOEMIdentifier);

    spdlog::info(L"tap-windows6 driver ({}) successfully added to the Windows driver store", adapterOEMIdentifier.toStdWString());

    return 100;
}

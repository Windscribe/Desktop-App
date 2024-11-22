#include "uninstall_info.h"

#include <filesystem>
#include <QDateTime>
#include <spdlog/spdlog.h>

#include "../settings.h"
#include "../../../utils/applicationInfo.h"
#include "../../../utils/path.h"

using namespace std;

UninstallInfo::UninstallInfo(double weight) : IInstallBlock(weight, L"uninstall_info")
{
}

UninstallInfo::~UninstallInfo()
{
}

int UninstallInfo::executeStep()
{
    registerUninstallInfo();
    return 100;
}

void UninstallInfo::registerUninstallInfo()
{
    // This will either open the existing key or create it.
    QSettings reg(QString::fromStdWString(ApplicationInfo::uninstallerRegistryKey()), QSettings::NativeFormat);
    reg.remove(""); // Clear out any existing key-value pairs.

    const wstring installPath = Settings::instance().getPath();
    const wstring uninstaller = Path::append(installPath, ApplicationInfo::uninstaller());

    setStringValue(reg, "InstallLocation", Path::addSeparator(installPath));
    setStringValue(reg, "DisplayName", ApplicationInfo::name());
    setStringValue(reg, "DisplayIcon", Path::append(installPath, ApplicationInfo::appExeName()));
    setStringValue(reg, "UninstallString", L"\"" + uninstaller + L"\"");
    setStringValue(reg, "QuietUninstallString", L"\"" + uninstaller + L"\" /SILENT");
    setStringValue(reg, "DisplayVersion", ApplicationInfo::version());
    setStringValue(reg, "Publisher", ApplicationInfo::publisher());
    setStringValue(reg, "URLInfoAbout", ApplicationInfo::publisherUrl());
    setStringValue(reg, "HelpLink", ApplicationInfo::supportUrl());
    setStringValue(reg, "URLUpdateInfo", ApplicationInfo::updateUrl());

    QString installDate = QDateTime::currentDateTime().toString("yyyyMMdd");
    setStringValue(reg, "InstallDate", installDate.toStdWString());

    error_code ec;
    uintmax_t totalFileSize = 0;
    for (auto const& dirEntry : filesystem::recursive_directory_iterator(installPath, ec)) {
        totalFileSize += dirEntry.file_size();
    }

    if (ec) {
        spdlog::error("UninstallInfo::registerUninstallInfo: filesystem::recursive_directory_iterator failed ({})", ec.message().c_str());
    }

    // EstimatedSize registry entry is expected to be a REG_DWORD.  Will get a REG_QWORD if we use uintmax_t.
    const uint estimatedSize = totalFileSize / 1024;
    reg.setValue("EstimatedSize", estimatedSize);
    reg.setValue("NoModify", 1);
    reg.setValue("NoRepair", 1);
}

void UninstallInfo::setStringValue(QSettings &reg, const char *key, const wstring &value)
{
    reg.setValue(key, QString::fromStdWString(value));
}

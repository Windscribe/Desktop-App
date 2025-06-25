#include "downloadhelper.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "names.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

#if defined(Q_OS_LINUX)
#include "utils/linuxutils.h"
#elif defined(Q_OS_MACOS)
#include "utils/utils.h"
#endif

using namespace wsnet;

DownloadHelper::DownloadHelper(QObject *parent, const QString &platform) : QObject(parent)
  , busy_(false)
  , platform_(platform)
  , downloadDirectory_(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
  , progressPercent_(0)
  , state_(DOWNLOAD_STATE_INIT)

{
    removeAutoUpdateInstallerFiles();
}

DownloadHelper::~DownloadHelper()
{
    deleteAllCurrentReplies();
}

const QString DownloadHelper::downloadInstallerPath()
{
#ifdef Q_OS_WIN
    const QString path = downloadInstallerPathWithoutExtension() + ".exe";
#elif defined Q_OS_MACOS
    const QString path = downloadInstallerPathWithoutExtension() + ".dmg";
#elif defined Q_OS_LINUX
    QString path;
    // if getPlatformName() fails, we should never get this far anyway
    if (platform_ == LinuxUtils::DEB_PLATFORM_NAME_X64 ||
        platform_ == LinuxUtils::DEB_PLATFORM_NAME_X64_CLI ||
        platform_ == LinuxUtils::DEB_PLATFORM_NAME_ARM64 ||
        platform_ == LinuxUtils::DEB_PLATFORM_NAME_ARM64_CLI) {
        path = downloadInstallerPathWithoutExtension() + ".deb";
    }
    else if (platform_ == LinuxUtils::RPM_PLATFORM_NAME_X64 ||
             platform_ == LinuxUtils::RPM_PLATFORM_NAME_X64_CLI ||
             platform_ == LinuxUtils::RPM_PLATFORM_NAME_ARM64 ||
             platform_ == LinuxUtils::RPM_PLATFORM_NAME_ARM64_CLI ||
             platform_ == LinuxUtils::RPM_OPENSUSE_PLATFORM_NAME ||
             platform_ == LinuxUtils::RPM_OPENSUSE_PLATFORM_NAME_CLI) {
        path = downloadInstallerPathWithoutExtension() + ".rpm";
    }
    else if (platform_ == LinuxUtils::ZST_PLATFORM_NAME ||
             platform_ == LinuxUtils::ZST_PLATFORM_NAME_CLI) {
        path = downloadInstallerPathWithoutExtension() + ".pkg.tar.zst";
    }
#endif
    WS_ASSERT(!path.isEmpty());
    return path;
}

const QString DownloadHelper::downloadInstallerPathWithoutExtension()
{
#ifdef Q_OS_WIN
    const QString path = downloadDirectory_ + "/installer";
#elif defined Q_OS_MACOS
    const QString path = downloadDirectory_ + "/installer";
#elif defined Q_OS_LINUX
    const QString path = downloadDirectory_ + "/update";
#endif
    return path;
}

void DownloadHelper::get(QMap<QString, QString> downloads)
{
    // assume get() should only run one set of downloads at a time
    if (busy_) {
        qCDebug(LOG_DOWNLOADER) << "Downloader is busy. Try again later";
        return;
    }

    busy_ = true;
    progressPercent_ = 0;
    for (const auto & download : downloads.keys())
        getInner(download, downloads[download]);
}

void DownloadHelper::stop()
{
    if (!busy_) {
        qCDebug(LOG_DOWNLOADER) << "No download to stop";
        return;
    }

    qCDebug(LOG_DOWNLOADER) << "Stopping download";
    deleteAllCurrentReplies();
    busy_ = false;
}

void DownloadHelper::onReplyFinished(std::uint64_t requestId, std::shared_ptr<WSNetRequestError> error, const std::string &data)
{
    auto it = replies_.find(requestId);
    if (it == replies_.end()) {
        return;
    }

    it->second->done = true;

    // if any reply fails, we fail
    if (!error->isSuccess()) {
        qCWarning(LOG_DOWNLOADER) << "Download failed";
        deleteAllCurrentReplies();
        busy_ = false;
        emit finished(DOWNLOAD_STATE_FAIL);
        return;
    }

    if (allRepliesDone()) {
        qCInfo(LOG_DOWNLOADER) << "Download finished successfully";
        deleteAllCurrentReplies();
        busy_ = false;
        emit finished(DOWNLOAD_STATE_SUCCESS);
        return;
    }

    // still waiting on replies
    qCInfo(LOG_DOWNLOADER) << "Download single file successful";
}

void DownloadHelper::onReplyDownloadProgress(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)
{
    auto it = replies_.find(requestId);
    if (it == replies_.end()) {
        return;
    }

    it->second->bytesReceived = bytesReceived;
    it->second->bytesTotal = bytesTotal;

    // recalculate total progress
    qint64 sum = 0;
    qint64 total = 0;
    for (const auto &reply : replies_) {
        sum += reply.second->bytesReceived;
        total += reply.second->bytesTotal;
    }

    progressPercent_ = (double) sum / (double) total * 100;
    emit progressChanged(progressPercent_);
}

void DownloadHelper::onReplyReadyRead(std::uint64_t requestId, const std::string &data)
{
    auto it = replies_.find(requestId);
    if (it == replies_.end()) {
        return;
    }
    if (!data.empty()) {
        if (it->second->file.isOpen()) {
            it->second->file.write(data.c_str(), data.size());
        } else {
            qCWarning(LOG_DOWNLOADER) << "Download error occurred (file not opened)";
        }
    }
}

void DownloadHelper::getInner(const QString url, const QString targetFilenamePath)
{
    // remove a previously used file if it exists
    QFile::remove(targetFilenamePath);
    qCDebug(LOG_DOWNLOADER) << "Starting download from url: " << url;

    auto fileAndProgess = std::make_unique<FileAndProgress>();
    fileAndProgess->file.setFileName(targetFilenamePath);
    if (!fileAndProgess->file.open(QIODevice::WriteOnly))
    {
        qCWarning(LOG_DOWNLOADER) << "Failed to open file for download" << url;
        return;
    }

    auto callbackFinished = [this] (std::uint64_t requestId, std::uint32_t elapsedMs,
                                    std::shared_ptr<WSNetRequestError> error, const std::string &data)
    {
        QMetaObject::invokeMethod(this, [this, requestId, error, data] {
            onReplyFinished(requestId, error, data);
        }) ;
    };

    auto callbackProgress = [this] (std::uint64_t requestId, std::uint64_t bytesReceived,
                                    std::uint64_t bytesTotal) {
        QMetaObject::invokeMethod(this, [this, requestId, bytesReceived, bytesTotal] {
            onReplyDownloadProgress(requestId, bytesReceived, bytesTotal);
        }) ;
    };

    auto callbackReadyData = [this] (std::uint64_t requestId, const std::string &data) {
        QMetaObject::invokeMethod(this, [this, requestId, data] { // NOLINT: false positive for memory leak
            onReplyReadyRead(requestId, data);
        }) ;
    };

    auto httpRequest = WSNet::instance()->httpNetworkManager()->createGetRequest(url.toStdString(), (std::uint16_t)(60000 * 10));  // timeout 10 mins
    httpRequest->setRemoveFromWhitelistIpsAfterFinish(true);

    fileAndProgess->request = WSNet::instance()->httpNetworkManager()->executeRequestEx(httpRequest, uniqueRequestId_, callbackFinished, callbackProgress, callbackReadyData);
    replies_[uniqueRequestId_] = std::move(fileAndProgess);
    uniqueRequestId_++;
}

void DownloadHelper::removeAutoUpdateInstallerFiles()
{
    // remove a previously used auto-update installer/dmg upon app startup if it exists
    const QString installerPath = downloadInstallerPath();
    if (QFile::exists(installerPath)) {
        qCDebug(LOG_DOWNLOADER) << "Removing auto-update installer";
        QFile::remove(installerPath);
    }

#ifdef Q_OS_MACOS
    // remove temp installer.app on mac:
    // | installer.app was unpacked from above .dmg
    const QString & installerApp = downloadDirectory_ + "/" + INSTALLER_FILENAME_MAC_APP;
    if (QFile::exists(installerApp)) {
        qCDebug(LOG_DOWNLOADER) << "Removing auto-update temporary installer app";
        Utils::removeDirectory(installerApp);
    }
#endif
}

bool DownloadHelper::allRepliesDone()
{
    for (const auto &reply : replies_) {
        // breaks
        if (!reply.second->done) {
            return false;
        }
    }
    return true;
}

void DownloadHelper::deleteAllCurrentReplies()
{
    for (const auto &reply : replies_)
        reply.second->request->cancel();

    replies_.clear();
}

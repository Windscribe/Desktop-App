#include "downloadhelper.h"

#include "utils/logger.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include "names.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "utils/utils.h"

#ifdef Q_OS_LINUX
#include "utils/linuxutils.h"
#endif

DownloadHelper::DownloadHelper(QObject *parent, NetworkAccessManager *networkAccessManager) : QObject(parent)
  , networkAccessManager_(networkAccessManager)
  , busy_(false)
  , progressPercent_(0)
  , state_(DOWNLOAD_STATE_INIT)
{
    // qCDebug(LOG_DOWNLOADER) << "Setting download location: " << Utils::cleanSensitiveInfo(path);
    downloadDirectory_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    removeAutoUpdateInstallerFiles();
}

const QString DownloadHelper::downloadInstallerPath()
{
#ifdef Q_OS_WIN
    const QString path = downloadInstallerPathWithoutExtension() + ".exe";
#elif defined Q_OS_MAC
    const QString path = downloadInstallerPathWithoutExtension() + ".dmg";
#elif defined Q_OS_LINUX
    QString path;
    if(LinuxUtils::isDeb()) {
        path = downloadInstallerPathWithoutExtension() + ".deb";
    }
    else {
        path = downloadInstallerPathWithoutExtension() + ".rpm";
    }
#endif
    return path;
}

const QString DownloadHelper::downloadInstallerPathWithoutExtension()
{
#ifdef Q_OS_WIN
    const QString path = downloadDirectory_ + "/installer";
#elif defined Q_OS_MAC
    const QString path = downloadDirectory_ + "/installer";
#elif defined Q_OS_LINUX
    const QString path = downloadDirectory_ + "/update";
#endif
    return path;
}

void DownloadHelper::get(QMap<QString, QString> downloads)
{
    // assume get() should only run one set of downloads at a time
    if (busy_)
    {
        qCDebug(LOG_DOWNLOADER) << "Downloader is busy. Try again later";
        return;
    }

    busy_ = true;
    progressPercent_ = 0;
    for (const auto & download : downloads.keys())
    {
        getInner(download, downloads[download]);
    }
}

void DownloadHelper::stop()
{
    if (!busy_)
    {
        qCDebug(LOG_DOWNLOADER) << "No download to stop";
        return;
    }

    qCDebug(LOG_DOWNLOADER) << "Stopping download";
    abortAllReplies();
    replies_.clear();
    busy_ = false;
}

DownloadHelper::DownloadState DownloadHelper::state()
{
    return state_;
}

void DownloadHelper::onReplyFinished()
{
    OptionalSharedNetworkReply optionalReply = replyFromSender(sender());
    if (!optionalReply.valid)
    {
        qCDebug(LOG_DOWNLOADER) << "Lost reply while download finishing";
        return;
    }
    const auto &reply = optionalReply.reply;

    // if any reply fails, we fail
    if (!reply->isSuccess())
    {
        qCDebug(LOG_DOWNLOADER) << "Download failed";
        abortAllReplies();
        replies_.clear();
        busy_ = false;
        emit finished(DOWNLOAD_STATE_FAIL);
        return;
    }

    if (allRepliesDone())
    {
        qCDebug(LOG_DOWNLOADER) << "Download finished successfully";
        replies_.clear();
        busy_ = false;
        emit finished(DOWNLOAD_STATE_SUCCESS);
        return;
    }

    // still waiting on replies
    qCDebug(LOG_DOWNLOADER) << "Download single file successful";
}

void DownloadHelper::onReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    OptionalSharedNetworkReply optionalReply = replyFromSender(sender());
    if (!optionalReply.valid)
    {
        qCDebug(LOG_DOWNLOADER) << "Lost reply while download progressing";
        return;
    }
    const auto &currentReply = optionalReply.reply;

    replies_[currentReply].bytesReceived = bytesReceived;
    replies_[currentReply].bytesTotal = bytesTotal;

    // recompute total progress
    qint64 sum = 0;
    qint64 total = 0;
    for (const auto & fileAndProgress : replies_.values())
    {
        sum += fileAndProgress.bytesReceived;
        total += fileAndProgress.bytesTotal;
    }

    // qCDebug(LOG_DOWNLOADER) << "Bytes received: " << bytesReceived << ", Bytes Total: " << bytesTotal;
    progressPercent_ = (double) sum / (double) total * 100;

    // qCDebug(LOG_DOWNLOADER) << "Downloading: " << progressPercent_;
    emit progressChanged(progressPercent_);
}

void DownloadHelper::onReplyReadyRead()
{
    OptionalSharedNetworkReply optionalReply = replyFromSender(sender());

    if (!optionalReply.valid)
    {
        qCDebug(LOG_DOWNLOADER) << "Lost reply while reading";
        return;
    }
    const auto &reply = optionalReply.reply;

    QByteArray arr = reply->readAll();

    if (!arr.isEmpty())
    {
        auto &file = replies_[reply].file;
        if (file->isOpen())
        {
            file->write(arr);
        }
        else
        {
            qCDebug(LOG_DOWNLOADER) << "Download error occurred (file not opened)";
        }
    }
}

void DownloadHelper::getInner(const QString url, const QString targetFilenamePath)
{
    // remove a previously used file if it exists
    QFile::remove(targetFilenamePath);
    qCDebug(LOG_DOWNLOADER) << "Starting download from url: " << url;

    FileAndProgress fileAndProgess;
    fileAndProgess.file = QSharedPointer<QFile>(new QFile(targetFilenamePath)); // unnecessary copy?
    if (!fileAndProgess.file->open(QIODevice::WriteOnly))
    {
        qCDebug(LOG_DOWNLOADER) << "Failed to open file for download" << url;
        return;
    }

    NetworkRequest request(QUrl(url), 60000 * 5, true);     // timeout 5 mins

    QSharedPointer<NetworkReply> reply = QSharedPointer<NetworkReply>(networkAccessManager_->get(request));
    replies_.insert(reply, fileAndProgess);
    connect(reply.get(), SIGNAL(finished()), SLOT(onReplyFinished()));
    connect(reply.get(), SIGNAL(progress(qint64,qint64)), SLOT(onReplyDownloadProgress(qint64,qint64)));
    connect(reply.get(), SIGNAL(readyRead()), SLOT(onReplyReadyRead()));
}

void DownloadHelper::removeAutoUpdateInstallerFiles()
{
    // remove a previously used auto-update installer/dmg upon app startup if it exists
    const QString installerPath = downloadInstallerPath();
    if (QFile::exists(installerPath))
    {
        qCDebug(LOG_DOWNLOADER) << "Removing auto-update installer";
        QFile::remove(installerPath);
    }

#ifdef Q_OS_LINUX
    // remove pre-existing signature and public key:
    // | signature and key were required to verify the auto-update installer
    const QString &signaturePath = signatureInstallPath();
    if (QFile::exists(signaturePath))
    {
        qCDebug(LOG_DOWNLOADER) << "Removing auto-update installer signature";
        QFile::remove(signaturePath);
    }
    const QString &publicKeyPath = publicKeyInstallPath();
    if (QFile::exists(publicKeyPath))
    {
        qCDebug(LOG_DOWNLOADER) << "Removing auto-update installer key";
        QFile::remove(publicKeyPath);
    }
#elif defined Q_OS_MAC
    // remove temp installer.app on mac:
    // | installer.app was unpacked from above .dmg
    const QString & installerApp = downloadDirectory_ + "/" + INSTALLER_FILENAME_MAC_APP;
    if (QFile::exists(installerApp))
    {
        qCDebug(LOG_DOWNLOADER) << "Removing auto-update temporary installer app";
        Utils::removeDirectory();
    }
#endif
}

bool DownloadHelper::allRepliesDone()
{
    for (const auto &reply : replies_.keys())
    {
        if (!reply->isDone())
        {
            return false;
        }
    }
    return true;
}

void DownloadHelper::abortAllReplies()
{
    for (const auto &reply : replies_.keys())
    {
        reply->abort();
    }
}

OptionalSharedNetworkReply DownloadHelper::replyFromSender(QObject *sender)
{
    NetworkReply *senderReply = static_cast<NetworkReply*>(sender);
    for (const auto &reply : replies_.keys())
    {
        if (reply.get() == senderReply)
        {
            return OptionalSharedNetworkReply(reply);
        }
    }
    return OptionalSharedNetworkReply();
}

const QString DownloadHelper::publicKeyInstallPath()
{
    return downloadInstallerPathWithoutExtension() + ".key";
}

const QString DownloadHelper::signatureInstallPath()
{
    return downloadInstallerPath() + ".sig";
}

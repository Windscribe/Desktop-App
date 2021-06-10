#include "downloadhelper.h"

#include "utils/logger.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include "names.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "utils/utils.h"

DownloadHelper::DownloadHelper(QObject *parent, NetworkAccessManager *networkAccessManager) : QObject(parent)
  , networkAccessManager_(networkAccessManager)
  , reply_(nullptr)
  , file_(nullptr)
  , progressPercent_(0)
  , state_(DOWNLOAD_STATE_INIT)
{
#ifdef Q_OS_WIN
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/installer.exe";
#elif defined Q_OS_MAC
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString path = tempDir + "/installer.dmg";

#elif defined Q_OS_LINUX
    //todo linux
    Q_ASSERT(false);
    const QString path;
#endif
    qCDebug(LOG_DOWNLOADER) << "Setting download location: " << Utils::cleanSensitiveInfo(path);
    downloadPath_ = path;

    // remove a previously used auto-update installer/dmg upon app startup if it exists
    QFile::remove(downloadPath_);

    // remove temp installer.app on mac
#ifdef Q_OS_MAC
    QString tempInstallerFilename = tempDir + "/" + INSTALLER_FILENAME_MAC_APP;
    Utils::removeDirectory(tempInstallerFilename);
#endif
}

const QString &DownloadHelper::downloadPath()
{
    return downloadPath_;
}

void DownloadHelper::get(const QString url)
{
    if (reply_)
    {
        qCDebug(LOG_DOWNLOADER) << "Downloader is busy. Try again later";
        return;
    }

    // remove a previously used auto-update installer/dmg upon app startup if it exists
    QFile::remove(downloadPath_);

    progressPercent_ = 0;

    qCDebug(LOG_DOWNLOADER) << "Starting download from url: " << url;

    file_ = new QFile(downloadPath_);
    if (!file_->open(QIODevice::WriteOnly))
    {
        qCDebug(LOG_DOWNLOADER) << "Failed to open file for download" << url;
        file_->deleteLater();
        file_ = nullptr;
        return;
    }

    NetworkRequest request(QUrl(url), 60000 * 5, true);     // timeout 5 mins

    reply_ = networkAccessManager_->get(request);
    connect(reply_, SIGNAL(finished()), SLOT(onReplyFinished()));
    connect(reply_, SIGNAL(progress(qint64,qint64)), SLOT(onReplyDownloadProgress(qint64,qint64)));
    connect(reply_, SIGNAL(readyRead()), SLOT(onReplyReadyRead()));
}

void DownloadHelper::stop()
{
    if (!reply_)
    {
        qCDebug(LOG_DOWNLOADER) << "No download to stop";
        return;
    }

    qCDebug(LOG_DOWNLOADER) << "Stopping download";
    reply_->abort();
    reply_->deleteLater();
    reply_ = nullptr;
}

DownloadHelper::DownloadState DownloadHelper::state()
{
    return state_;
}

void DownloadHelper::onReplyFinished()
{
    DownloadState state;
    if (reply_->isSuccess())
    {
        qCDebug(LOG_DOWNLOADER) << "Download finished successfully";
        state = DOWNLOAD_STATE_SUCCESS;
    }
    else
    {
        qCDebug(LOG_DOWNLOADER) << "Download failed";
        state = DOWNLOAD_STATE_FAIL;
    }
    safeCloseFileAndDeleteObj();
    emit finished(state);

    reply_->deleteLater();
    reply_ = nullptr;
}

void DownloadHelper::onReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    // qCDebug(LOG_DOWNLOADER) << "Bytes received: " << bytesReceived << ", Bytes Total: " << bytesTotal;
    progressPercent_ = (double) bytesReceived / (double) bytesTotal * 100;

    // qCDebug(LOG_DOWNLOADER) << "Downloading: " << progressPercent_;
    emit progressChanged(progressPercent_);
}

void DownloadHelper::onReplyReadyRead()
{
    QByteArray arr = reply_->readAll();

    if (!arr.isEmpty())
    {
        if (file_->isOpen())
        {
            file_->write(arr);
        }
        else
        {
            qCDebug(LOG_DOWNLOADER) << "Download error occurred (file not opened)";
        }
    }
}

void DownloadHelper::safeCloseFileAndDeleteObj()
{
    if (file_)
    {
        file_->close();
        file_->deleteLater();
        file_ = nullptr;
    }
}

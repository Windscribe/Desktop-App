#include "downloadhelper.h"

#include "utils/logger.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include "names.h"
#include "utils/utils.h"

DownloadHelper::DownloadHelper(QObject *parent) : QObject(parent)
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

#endif
    qCDebug(LOG_DOWNLOADER) << "Setting download location: " << path;
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

    QNetworkRequest request(url);

    QNetworkReply *reply = networkAccessManager_.get(request);
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(onReplyDownloadProgress(qint64,qint64)));
    connect(reply, SIGNAL(finished()), SLOT(onReplyFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(onReplyError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(readyRead()), SLOT(onReplyReadyRead()));
    reply_ = reply;
}

void DownloadHelper::stop()
{
    if (!reply_)
    {
        qCDebug(LOG_DOWNLOADER) << "No download to stop";
        return;
    }

    qCDebug(LOG_DOWNLOADER) << "Stopping download";
    reply_->abort(); // should fire the finished signal for cleanup
}

DownloadHelper::DownloadState DownloadHelper::state()
{
    return state_;
}

void DownloadHelper::onReplyFinished()
{
    DownloadState state;
    if (reply_->error() == QNetworkReply::NoError)
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

void DownloadHelper::onReplyError(QNetworkReply::NetworkError error)
{
    qCDebug(LOG_DOWNLOADER) << "Download error occurred: " << error;
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

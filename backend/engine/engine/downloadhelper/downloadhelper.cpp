#include "downloadhelper.h"

#include "utils/logger.h"
#include <QStandardPaths>

DownloadHelper::DownloadHelper(QObject *parent) : QObject(parent)
  , reply_(nullptr)
{
#ifdef Q_OS_WIN
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/installer.exe";
#elif defined Q_OS_MAC

#endif
    qCDebug(LOG_DOWNLOADER) << "Setting download location: " << path;
    downloadPath_ = path;
}

const QString DownloadHelper::downloadPath()
{
    return downloadPath_;
}

void DownloadHelper::setDownloadPath(const QString path)
{
    downloadPath_ = path;
}

void DownloadHelper::get(const QString url)
{
    if (reply_)
    {
        qCDebug(LOG_DOWNLOADER) << "Downloader is busy. Try again later";
        return;
    }

    qCDebug(LOG_DOWNLOADER) << "Starting download from url: " << url;

    file_ = new QFile(downloadPath_);
    file_->open(QIODevice::WriteOnly);

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

uint DownloadHelper::progressPercent()
{
    return progressPercent_;
}

void DownloadHelper::onReplyFinished()
{
    safeCloseFileAndDeleteObj();

    if (reply_->error() == QNetworkReply::NoError)
    {
        qCDebug(LOG_DOWNLOADER) << "Download finished successfully";
        emit finished(DOWNLOAD_STATE_SUCCESS);
    }
    else
    {
        qCDebug(LOG_DOWNLOADER) << "Download failed";
        emit finished(DOWNLOAD_STATE_FAIL);
    }

    reply_->deleteLater();
    reply_ = nullptr;
}

void DownloadHelper::onReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
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
    //qCDebug(LOG_DOWNLOADER) << "Ready reading...";
    QByteArray arr = reply_->readAll();

    if (file_->isOpen())
    {
        //qCDebug(LOG_DOWNLOADER) << "Writing...";
        file_->write(arr);
    }
}

void DownloadHelper::safeCloseFileAndDeleteObj()
{
    if (file_)
    {
        if (file_->isOpen())
        {
            file_->close();
        }

        file_->deleteLater();
        file_ = nullptr;
    }
}

#pragma once

#include <QString>
#include <QObject>
#include <QFile>
#include <QSharedPointer>
#include <QMap>
#include <wsnet/WSNet.h>

class DownloadHelper : public QObject
{
    Q_OBJECT
public:
    explicit DownloadHelper(QObject *parent, const QString &platform);
    ~DownloadHelper();

    enum DownloadState {
        DOWNLOAD_STATE_INIT,
        DOWNLOAD_STATE_RUNNING,
        DOWNLOAD_STATE_SUCCESS,
        DOWNLOAD_STATE_FAIL
    };

    const QString downloadInstallerPath();
    const QString downloadInstallerPathWithoutExtension();

    void get(QMap<QString, QString> downloads);
    void stop();

signals:
    void finished(DownloadHelper::DownloadState state);
    void progressChanged(uint progressPercent);

private:
    std::uint64_t uniqueRequestId_ = 0;

    struct FileAndProgress {
        std::shared_ptr<wsnet::WSNetCancelableCallback> request;
        QFile file;
        qint64 bytesReceived = 0;
        qint64 bytesTotal = 0;
        bool done = false;
    };

    std::map<std::uint64_t, std::unique_ptr<FileAndProgress> > replies_;
    bool busy_;
    const QString platform_;

    QString downloadDirectory_;
    uint progressPercent_;
    DownloadState state_;

    void getInner(const QString url, const QString targetFilenamePath);
    void removeAutoUpdateInstallerFiles();
    bool allRepliesDone();
    void deleteAllCurrentReplies();

    void onReplyFinished(std::uint64_t requestId,
                         wsnet::NetworkError errCode, const std::string &data);
    void onReplyDownloadProgress(std::uint64_t requestId, std::uint64_t bytesReceived,
                                 std::uint64_t bytesTotal);
    void onReplyReadyRead(std::uint64_t requestId, const std::string &data);
};

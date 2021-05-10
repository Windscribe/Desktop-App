#ifndef DOWNLOADHELPER_H
#define DOWNLOADHELPER_H

#include <QString>
#include <QObject>
#include <QFile>

class NetworkAccessManager;
class NetworkReply;

class DownloadHelper : public QObject
{
    Q_OBJECT
public:
    explicit DownloadHelper(QObject *parent, NetworkAccessManager *networkAccessManager);

    enum DownloadState {
        DOWNLOAD_STATE_INIT,
        DOWNLOAD_STATE_RUNNING,
        DOWNLOAD_STATE_SUCCESS,
        DOWNLOAD_STATE_FAIL
    };

    const QString &downloadPath();
    void get(const QString url);
    void stop();

    DownloadState state();

signals:
    void finished(DownloadHelper::DownloadState state);
    void progressChanged(uint progressPercent);

private slots:
    void onReplyFinished();
    void onReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReplyReadyRead();

private:
    NetworkAccessManager *networkAccessManager_;
    NetworkReply *reply_;
    QFile *file_;

    QString downloadPath_;
    uint progressPercent_;
    DownloadState state_;

    void safeCloseFileAndDeleteObj();
};

#endif // DOWNLOADHELPER_H

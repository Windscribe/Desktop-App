#ifndef LOCALHTTPSERVER_H
#define LOCALHTTPSERVER_H

#include <QObject>
#include <QProcess>
#include <functional>

typedef std::function<bool(QString, bool, bool, QString)> LocalHttpServerCallback;

class LocalHttpServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalHttpServer(QObject *parent, const LocalHttpServerCallback &callbackCommand);
    ~LocalHttpServer() override;

    void startServer();

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onReadyReadStandardOutput();
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    LocalHttpServerCallback funcCallback_;
    QProcess *process_;
    QByteArray inputArr_;

    void sendFinishCommandToProcess();
    QString getNextStringFromInputBuffer(bool &bSuccess, int &outSize);
    void handleCommandString(const QString &str);
};

#endif // LOCALHTTPSERVER_H

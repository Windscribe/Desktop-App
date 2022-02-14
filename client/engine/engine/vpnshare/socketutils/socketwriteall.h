#ifndef SOCKETWRITEALL_H
#define SOCKETWRITEALL_H

#include <QObject>
#include <QTcpSocket>

class SocketWriteAll : public QObject
{
    Q_OBJECT
public:
    explicit SocketWriteAll(QObject *parent, QTcpSocket *socket);
    void write(const QByteArray &arr);

    void setEmitAllDataWritten();

signals:
    void allDataWriteFinished();

private slots:
    void onBytesWritten(qint64 bytes);

private:
    QTcpSocket *socket_;
    QByteArray arr_;
    bool bEmitAllDataWritten_;
};

#endif // SOCKETWRITEALL_H

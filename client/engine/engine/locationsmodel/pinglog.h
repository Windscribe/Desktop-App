#ifndef PINGLOG_H
#define PINGLOG_H

#include <QFile>
#include <QTextStream>
#include <QMutex>

class PingLog
{
public:
    explicit PingLog(const QString &filename);
    PingLog(const PingLog &) = delete;
    PingLog &operator=(const PingLog &) = delete;
    ~PingLog();

    void addLog(const QString &tag, const QString &str);

private:
    QMutex mutex_;
    QFile *file_;
    QTextStream textStream_;
};

#endif // PINGLOG_H

#include "pinglog.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

PingLog::PingLog(const QString &filename) : file_(NULL)
{
    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(logFilePath);
    dir.mkpath(logFilePath);
    logFilePath += "/" + filename;
    file_ = new QFile(logFilePath);
    if (file_->open(QIODeviceBase::WriteOnly))
    {
        textStream_.setDevice(file_);
    }
}

PingLog::~PingLog()
{
    textStream_.setDevice(NULL);
    if (file_)
    {
        file_->close();
        delete file_;
    }
}

void PingLog::addLog(const QString &tag, const QString &str)
{
    QMutexLocker locker(&mutex_);
    QDateTime dt = QDateTime::currentDateTime();
    textStream_ << dt.toString("ddMMyyyy HH:mm:ss") << "\t" << tag << "\t\t" << str << "\n";
    //qDebug() << tag << "\t\t" << str;
    textStream_.flush();
}

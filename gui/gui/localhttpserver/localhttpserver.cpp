#include "localhttpserver.h"
#include "Utils/logger.h"

#include <QApplication>

LocalHttpServer::LocalHttpServer(QObject *parent, const LocalHttpServerCallback &callbackCommand) : QObject(parent),
    funcCallback_(callbackCommand), process_(NULL)
{
}

LocalHttpServer::~LocalHttpServer()
{
    if (process_)
    {
        sendFinishCommandToProcess();
        process_->waitForFinished(1000);
    }
}

void LocalHttpServer::startServer()
{
#ifdef Q_OS_WIN
    QString appPath = QApplication::applicationDirPath();
    QString exePath = appPath + "/wsappcontrol.exe";
#elif defined Q_OS_MAC
    QString appPath = QApplication::applicationDirPath() + "/../Library";
    QString exePath = appPath + "/wsappcontrol";
#endif

    Q_ASSERT(process_ == NULL);
    process_ = new QProcess(this);
    connect(process_, SIGNAL(started()), SLOT(onProcessStarted()));
    connect(process_, SIGNAL(finished(int)), SLOT(onProcessFinished()));
    connect(process_, SIGNAL(readyReadStandardOutput()), SLOT(onReadyReadStandardOutput()));
    connect(process_, SIGNAL(errorOccurred(QProcess::ProcessError)), SLOT(onProcessErrorOccurred(QProcess::ProcessError)));
    process_->setWorkingDirectory(appPath);
    process_->start(exePath, QStringList());
}

void LocalHttpServer::onProcessStarted()
{
    qCDebug(LOG_LOCAL_WEBSERVER) << "Local HTTPS webserver process started";
}

void LocalHttpServer::onProcessFinished()
{
    qCDebug(LOG_LOCAL_WEBSERVER) << "Local HTTPS webserver process finished";
}

void LocalHttpServer::onReadyReadStandardOutput()
{
    inputArr_.append(process_->readAll());
    bool bSuccess = true;
    int length;
    while (true)
    {
        QString str = getNextStringFromInputBuffer(bSuccess, length);
        if (bSuccess)
        {
            inputArr_.remove(0, length);
            handleCommandString(str);
        }
        else
        {
            break;
        }
    };
}

void LocalHttpServer::onProcessErrorOccurred(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    qCDebug(LOG_LOCAL_WEBSERVER) << "Local HTTPS webserver error:" << process_->errorString();
}

void LocalHttpServer::sendFinishCommandToProcess()
{
    process_->write("exit\n");
}

QString LocalHttpServer::getNextStringFromInputBuffer(bool &bSuccess, int &outSize)
{
    QString str;
    bSuccess = false;
    outSize = 0;
    for (int i = 0; i < inputArr_.size(); ++i)
    {
        if (inputArr_[i] == '\n')
        {
            bSuccess = true;
            outSize = i + 1;
            return str.trimmed();
        }
        str += inputArr_[i];
    }

    return QString();
}

void LocalHttpServer::handleCommandString(const QString &str)
{
    if (str.startsWith("LOG: "))
    {
        qCDebug(LOG_LOCAL_WEBSERVER) << str.mid(5);
    }
    else if (str.startsWith("CMD: "))
    {
        QStringList strs = str.mid(5).split(",");
        if (strs.size() != 4)
        {
            qCDebug(LOG_LOCAL_WEBSERVER) << "Unknown output from webserver process:" << str;
        }
        else
        {
            qCDebug(LOG_LOCAL_WEBSERVER) << str.mid(5);
            QString authHash = strs[0];
            bool isConnectCmd = strs[1].toInt() != 0;
            bool isDisconnectCmd = strs[2].toInt() != 0;
            QString location = strs[3];
            bool b = funcCallback_(authHash, isConnectCmd, isDisconnectCmd, location);
            if (b)
            {
                process_->write("true\n");
            }
            else
            {
                process_->write("false\n");
            }
        }
    }
    else
    {
        qCDebug(LOG_LOCAL_WEBSERVER) << "Unknown output from webserver process:" << str;
    }
}

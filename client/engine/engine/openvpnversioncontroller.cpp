#include "openvpnversioncontroller.h"

#include <QDir>
#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QRegularExpression>

QStringList OpenVpnVersionController::getAvailableOpenVpnVersions()
{
    QMutexLocker locker(&mutex_);
    return openVpnVersionsList_;
}

QStringList OpenVpnVersionController::getAvailableOpenVpnExecutables()
{
    QMutexLocker locker(&mutex_);
    return openVpnFilesList_;
}

QString OpenVpnVersionController::getSelectedOpenVpnVersion()
{
    QMutexLocker locker(&mutex_);
    if (selectedInd_ >= 0 && selectedInd_ < openVpnVersionsList_.count())
    {
        return openVpnVersionsList_[selectedInd_];
    }
    else
    {
        return "";
    }
}

QString OpenVpnVersionController::getSelectedOpenVpnExecutable()
{
    QMutexLocker locker(&mutex_);
    if (selectedInd_ >= 0 && selectedInd_ < openVpnVersionsList_.count())
    {
        return openVpnFilesList_[selectedInd_];
    }
    else
    {
        return "";
    }
}

void OpenVpnVersionController::setSelectedOpenVpnVersion(const QString &version)
{
    QMutexLocker locker(&mutex_);
    for (int i = 0; i < openVpnVersionsList_.count(); ++i)
    {
        if (openVpnVersionsList_[i] == version)
        {
            if (selectedInd_ != i)
            {
                selectedInd_ = i;
                QSettings settings;
                settings.setValue("openvpnVersion", version);
            }
            break;
        }
    }
}

void OpenVpnVersionController::setUseWinTun(bool bUseWinTun)
{
    bUseWinTun_ = bUseWinTun;
    if (bUseWinTun_)
    {
        setSelectedOpenVpnVersion("2.5.0");
    }
    else
    {
        setSelectedOpenVpnVersion("2.5.0");
    }
}

bool OpenVpnVersionController::isUseWinTun()
{
    return bUseWinTun_;
}

OpenVpnVersionController::OpenVpnVersionController()
{
    bUseWinTun_ = false;
    selectedInd_ = -1;
    QString pathDir = getOpenVpnBinaryPath();
    QDir dir(pathDir);
    const QStringList files = dir.entryList(QStringList() << "windscribeopenvpn*");
    for (const QString &fileName : files)
    {
        QString version = detectVersion(pathDir + "/" + fileName);
        if (!version.isEmpty())
        {
            openVpnVersionsList_ << version;
            openVpnFilesList_ << fileName;
        }
    }

    QSettings settings;
    QString selVersion = settings.value("openvpnVersion", "").toString();
    if (!selVersion.isEmpty())
    {
        for (int i = 0; i < openVpnVersionsList_.count(); ++i)
        {
            if (openVpnVersionsList_[i] == selVersion)
            {
                selectedInd_ = i;
                break;
            }
        }
    }
    // if no saved selected version, select default
    if (selectedInd_ == -1)
    {
        if (openVpnVersionsList_.count() == 1)
        {
            selectedInd_ = 0;
        }
        else
        {
            for (int i = 0; i < openVpnVersionsList_.count(); ++i)
            {
                if (openVpnVersionsList_[i] == "2.4.8")
                {
                    selectedInd_ = i;
                    settings.setValue("openvpnVersion", openVpnVersionsList_[i]);
                    break;
                }
            }
        }
    }
    // last chance, if openVpnVersionsList_.count > 0, select first
    if (selectedInd_ == -1 && openVpnVersionsList_.count() > 0)
    {
        selectedInd_ = 0;
    }
}

QString OpenVpnVersionController::getOpenVpnBinaryPath()
{
#ifdef Q_OS_WIN
    QString path = QCoreApplication::applicationDirPath();
#elif defined Q_OS_MAC
    QString path = QCoreApplication::applicationDirPath() + "/../Helpers";
#elif defined Q_OS_LINUX
    QString path = QCoreApplication::applicationDirPath();
#endif
    return path;
}

QString OpenVpnVersionController::detectVersion(const QString &path)
{
    if (!QFile::exists(path))
    {
        return "";
    }
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(path, QStringList() << "--version");
    process.waitForFinished(-1);
    QString strAnswer = QString::fromStdString((const char *)process.readAll().data()).toLower();

    // parse version from process output
    QRegularExpression rx("\\d{1,}.\\d{1,}.\\d{1,}");
    QRegularExpressionMatch match = rx.match(strAnswer);
    QStringList list = match.capturedTexts();
    Q_ASSERT(list.count() == 1);
    if (list.count() == 1)
    {
        return list[0];
    }
    else
    {
        return "";
    }
}

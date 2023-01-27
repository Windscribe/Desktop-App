#include "helper_mac.h"
#include "utils/logger.h"
#include <QStandardPaths>
#include "utils/network_utils/network_utils_mac.h"
#include "utils/macutils.h"
#include <QCoreApplication>
#include "installhelper_mac.h"
#include "../../../../backend/posix_common/helper_commands_serialize.h"

Helper_mac::Helper_mac(QObject *parent) : Helper_posix(parent)
{

}

Helper_mac::~Helper_mac()
{

}

void Helper_mac::startInstallHelper()
{
    bool isUserCanceled;
    if (InstallHelper_mac::installHelper(isUserCanceled))
    {
        start(QThread::LowPriority);
    }
    else
    {
        if (isUserCanceled)
        {
            curState_ = STATE_USER_CANCELED;
        }
        else
        {
            curState_ = STATE_INSTALL_FAILED;
        }
    }
}

bool Helper_mac::reinstallHelper()
{
    QString strUninstallUtilPath = QCoreApplication::applicationDirPath() + "/../Resources/uninstallHelper.sh";
    InstallHelper_mac::runScriptWithAdminRights(strUninstallUtilPath);
    return true;
}


QString Helper_mac::getHelperVersion()
{
    return "";
}

bool Helper_mac::setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    Q_UNUSED(ifIndex)
    Q_UNUSED(overrideDnsIpAddress)

    // get list of entries of interest
    QStringList networkServices = NetworkUtils_mac::getListOfDnsNetworkServiceEntries();

    // filter list to only SetByWindscribe entries
    QStringList dnsNetworkServices;

    if (isIkev2)
    {
        // IKEv2 is slightly different -- look for "ConfirmedServiceID" key in each DNS dictionary
        for (QString service : networkServices)
        {
            if (MacUtils::dynamicStoreEntryHasKey(service, "ConfirmedServiceID"))
            {
                dnsNetworkServices.append(service);
            }
        }
    }
    else
    {
        // WG and openVPN: just look for 'SetByWindscribe' key in each DNS dictionary
        for (QString service : networkServices)
        {
            if (MacUtils::dynamicStoreEntryHasKey(service, "SetByWindscribe"))
            {
                dnsNetworkServices.append(service);
            }
        }
    }
    qCDebug(LOG_CONNECTED_DNS) << "Applying custom 'while connected' DNS change to network services: " << dnsNetworkServices;

    if (dnsNetworkServices.isEmpty())
    {
        qCDebug(LOG_CONNECTED_DNS) << "No network services to confirgure 'while connected' DNS";
        return false;
    }

    // change DNS on each entry
    bool successAll = true;
    for (QString service : dnsNetworkServices)
    {
        if (!Helper_mac::setDnsOfDynamicStoreEntry(overrideDnsIpAddress, service))
        {
            successAll = false;
            qCDebug(LOG_CONNECTED_DNS) << "Failed to set network service DNS: " << service;
            break;
        }
    }

    return successAll;
}



void Helper_mac::enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress)
{
    qCDebug(LOG_BASIC) << "Enable MAC spoofing on boot, bEnable =" << bEnable;
    QString strTempFilePath = QString::fromLocal8Bit(getenv("TMPDIR")) + "windscribetemp.plist";
    QString filePath = "/Library/LaunchDaemons/com.aaa.windscribe.macspoofing_on.plist";

    QString macSpoofingScript = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/windscribe_macspoof.sh";

    if (bEnable)
    {
        // bash script
        {
            QString exePath = QCoreApplication::applicationFilePath();
            QFile file(macSpoofingScript);
            if (file.open(QIODeviceBase::WriteOnly | QIODevice::Text))
            {
                file.resize(0);
                QTextStream in(&file);
                in << "#!/bin/bash\n";
                in << "FILE=\"" << exePath << "\"\n";
                in << "if [ ! -f \"$FILE\" ]\n";
                in << "then\n";
                in << "echo \"File $FILE does not exists\"\n";
                in << "launchctl stop com.aaa.windscribe.macspoofing_on\n";
                in << "launchctl unload " << filePath << "\n";
                in << "launchctl remove com.aaa.windscribe.macspoofing_on\n";
                in << "srm \"$0\"\n";
                //in << "rm " << filePath << "\n";
                in << "else\n";
                in << "echo \"File $FILE exists\"\n";
                in << "ipconfig waitall\n";
                in << "sleep 3\n";
                in << "/sbin/ifconfig " << interfaceName << " ether " << macAddress << "\n";
                in << "fi\n";
                file.close();

                // set executable flag
                executeRootCommand("chmod +x \"" + macSpoofingScript + "\"");
            }
        }

        // create plist
        QFile file(strTempFilePath);
        if (file.open(QIODeviceBase::WriteOnly | QIODevice::Text))
        {
            file.resize(0);
            QTextStream in(&file);
            in << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            in << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
            in << "<plist version=\"1.0\">\n";
            in << "<dict>\n";
            in << "<key>Label</key>\n";
            in << "<string>com.aaa.windscribe.macspoofing_on</string>\n";

            in << "<key>ProgramArguments</key>\n";
            in << "<array>\n";
            in << "<string>/bin/bash</string>\n";
            in << "<string>" << macSpoofingScript << "</string>\n";
            in << "</array>\n";

            in << "<key>StandardErrorPath</key>\n";
            in << "<string>/var/log/windscribe_macspoofing.log</string>\n";
            in << "<key>StandardOutPath</key>\n";
            in << "<string>/var/log/windscribe_macspoofing.log</string>\n";

            in << "<key>RunAtLoad</key>\n";
            in << "<true/>\n";
            in << "</dict>\n";
            in << "</plist>\n";

            file.close();

            executeRootCommand("cp " + strTempFilePath + " " + filePath);
            executeRootCommand("launchctl load -w " + filePath);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Can't create plist file for startup firewall: " << filePath;
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "launchctl unload " + Utils::cleanSensitiveInfo(filePath);
        executeRootCommand("launchctl unload " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "rm " + Utils::cleanSensitiveInfo(filePath);
        executeRootCommand("rm " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "rm " + Utils::cleanSensitiveInfo(macSpoofingScript);
        executeRootCommand("rm \"" + macSpoofingScript + "\"");
    }
}

QStringList Helper_mac::getActiveNetworkInterfaces()
{
    const QString answer = executeRootCommand("ifconfig -a");
    const QStringList lines = answer.split("\n");
    QStringList res;
    for (const QString &s : lines)
    {
        if (s.startsWith("en", Qt::CaseInsensitive))
        {
            int ind = s.indexOf(':');
            if (ind != -1)
            {
                res << s.left(ind);
            }
        }
    }

    return res;
}

bool Helper_mac::setKeychainUsernamePassword(const QString &username, const QString &password)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
    {
        return false;
    }

    bool bExecuted;
    int ret = setKeychainUsernamePasswordImpl(username, password, &bExecuted);
    if (ret == RET_SUCCESS)
    {
        return bExecuted;
    }
    else
    {
        qCDebug(LOG_BASIC) << "App disconnected from helper, try reconnect";
        doDisconnectAndReconnect();

        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        while (elapsedTimer.elapsed() < MAX_WAIT_HELPER && !isNeedFinish())
        {
            if (curState_ == STATE_CONNECTED)
            {
                ret = setKeychainUsernamePasswordImpl(username, password, &bExecuted);
                if (ret == RET_SUCCESS)
                {
                    return bExecuted;
                }
            }
            msleep(1);
        }
    }

    qCDebug(LOG_BASIC) << "setKeychainUsernamePassword() failed";
    return false;
}

bool Helper_mac::setKextPath(const QString &kextPath)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
    {
        return false;
    }

    CMD_SET_KEXT_PATH cmd;
    cmd.kextPath = kextPath.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_SET_KEXT_PATH, stream.str()))
    {
        return false;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            return false;
        }
    }
    return true;
}

bool Helper_mac::setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &entry)
{
    QMutexLocker locker(&mutex_);

    CMD_APPLY_CUSTOM_DNS cmd;
    cmd.ipAddress = ipAddress.toStdString();
    cmd.networkService = entry.toStdString();

    if (curState_ != STATE_CONNECTED)
        return false;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_APPLY_CUSTOM_DNS, stream.str()))
    {
        return false;
    }

    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd))
    {
        return false;
    }

    return answerCmd.executed != 0;
}


bool Helper_mac::setKeychainUsernamePasswordImpl(const QString &username, const QString &password, bool *bExecuted)
{
    CMD_SET_KEYCHAIN_ITEM cmd;
    cmd.username = username.toStdString();
    cmd.password = password.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_SET_KEYCHAIN_ITEM, stream.str()))
    {
        return RET_DISCONNECTED;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            return RET_DISCONNECTED;
        }
        else
        {
            *bExecuted = (answerCmd.executed == 1);
            return RET_SUCCESS;
        }
    }
}

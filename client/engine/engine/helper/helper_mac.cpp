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
    InstallHelper_mac::uninstallHelper();
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

bool Helper_mac::setMacAddress(const QString &interface, const QString &macAddress, bool robustMethod)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_MAC_ADDRESS cmd;
    CMD_ANSWER answer;
    cmd.interface = interface.toStdString();
    cmd.macAddress = macAddress.toStdString();
    cmd.robustMethod = robustMethod;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_MAC_ADDRESS, stream.str(), answer);

}

bool Helper_mac::enableMacSpoofingOnBoot(bool bEnabled, const QString &interface, const QString &macAddress, bool robustMethod)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_MAC_SPOOFING_ON_BOOT cmd;
    CMD_ANSWER answer;
    cmd.enabled = bEnabled;
    cmd.interface = interface.toStdString();
    cmd.macAddress = macAddress.toStdString();
    cmd.robustMethod = robustMethod;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_MAC_SPOOFING_ON_BOOT, stream.str(), answer);
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

bool Helper_mac::setIpv6Enabled(bool bEnabled)
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    CMD_SET_IPV6_ENABLED cmd;
    cmd.enabled = bEnabled;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_IPV6_ENABLED, stream.str(), answer);
}

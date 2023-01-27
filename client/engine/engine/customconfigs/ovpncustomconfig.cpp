#include "ovpncustomconfig.h"
#include "utils/logger.h"
#include "parseovpnconfigline.h"

#include <QFileInfo>

namespace customconfigs {

namespace {

bool CheckAndFixProtocolName(QString &protocol)
{
    bool is_valid = false;
    if (protocol.startsWith("udp")) {
        is_valid = !protocol.startsWith("udp6");
        protocol = "udp";
    } else if (protocol.startsWith("tcp")) {
        is_valid = !protocol.startsWith("tcp6");
        protocol = "tcp";
    }
    return is_valid;
}

}  // namespace

CUSTOM_CONFIG_TYPE OvpnCustomConfig::type() const
{
    return CUSTOM_CONFIG_OPENVPN;
}

QString OvpnCustomConfig::name() const
{
    return name_;
}

QString OvpnCustomConfig::nick() const
{
    return nick_;
}

QString OvpnCustomConfig::filename() const
{
    return filename_;
}

QStringList OvpnCustomConfig::hostnames() const
{
    QStringList list;
    for (auto r : remotes_)
    {
        list << r.hostname;
    }
    return list;
}

bool OvpnCustomConfig::isAllowFirewallAfterConnection() const
{
    return isAllowFirewallAfterConnection_;
}

bool OvpnCustomConfig::isCorrect() const
{
    return isCorrect_;
}

QString OvpnCustomConfig::getErrorForIncorrect() const
{
    return errMessage_;
}

ICustomConfig *OvpnCustomConfig::makeFromFile(const QString &filepath)
{
    OvpnCustomConfig *config = new OvpnCustomConfig();
    QFileInfo fi(filepath);
    config->name_ = fi.completeBaseName();
    config->filename_ = fi.fileName();
    config->filepath_ = filepath;
    config->globalPort_ = 0;
    config->isCorrect_ = true;      // by default correct config
    config->process();              // here the config can change to incorrect
    return config;
}

QVector<RemoteCommandLine> OvpnCustomConfig::remotes() const
{
    return remotes_;
}

uint OvpnCustomConfig::globalPort() const
{
    return globalPort_;
}

QString OvpnCustomConfig::globalProtocol() const
{
    return globalProtocol_;
}

QString OvpnCustomConfig::getOvpnData() const
{
    return ovpnData_;
}

// retrieves all hostnames/IPs "remote ..." commands
// also ovpn-config with removed "remote" commands saves in ovpnData_
// removed "remote" commands are saved in remotes_ (to restore the config with changed hostnames/IPs before connect)
void OvpnCustomConfig::process()
{
#ifdef Q_OS_LINUX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

    QFile file(filepath_);
    if (file.open(QIODeviceBase::ReadOnly))
    {
        qDebug(LOG_CUSTOM_OVPN) << "Opened:" << Utils::cleanSensitiveInfo(filepath_);

        bool bFoundAtLeastOneRemote = false;
        bool bFoundVerbCommand = false;
        bool bFoundScriptSecurityCommand = false;
        bool isTapDevice = false;
        bool bHasValidCipher = false;
        QString currentProtocol{ "udp" };
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            ParseOvpnConfigLine::OpenVpnLine openVpnLine = ParseOvpnConfigLine::processLine(line);
            if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_REMOTE_IP) // remote ip
            {
                RemoteCommandLine rcl;
                rcl.hostname = openVpnLine.host;
                rcl.originalRemoteCommand = line;
                rcl.port = openVpnLine.port;
                rcl.protocol = openVpnLine.protocol;
                remotes_ << rcl;
                qDebug(LOG_CUSTOM_OVPN) << "Extracted hostname/IP:" << openVpnLine.host << " from  remote cmd:" << line;
                if (!openVpnLine.protocol.isEmpty())
                {
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted protocol:" << openVpnLine.protocol << " from  remote cmd:" << line;
                }
                if (openVpnLine.port != 0)
                {
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted port:" << openVpnLine.port << " from  remote cmd:" << line;
                }

                bFoundAtLeastOneRemote = true;
            }
            else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_VERB) // verb cmd
            {
                qDebug(LOG_CUSTOM_OVPN) << "Extracted verb:" << openVpnLine.verb;
                if (openVpnLine.verb < 3)
                    openVpnLine.verb = 3;

                ovpnData_ += "verb " + QString::number(openVpnLine.verb) + "\n";
                bFoundVerbCommand = true;
            }
            else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_SCRIPT_SECURITY) // script-security cmd
            {
                qDebug(LOG_CUSTOM_OVPN) << "Extracted script-security:" << openVpnLine.verb;
#ifdef Q_OS_MAC
                // Needed script-security at least 2 on Mac, to allow an "up" command for the DNS
                // setup script.
                if (openVpnLine.verb < 2)
                    openVpnLine.verb = 2;
#endif
                ovpnData_ += "script-security " + QString::number(openVpnLine.verb) + "\n";
                bFoundScriptSecurityCommand = true;
            }
            else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_CIPHER) // cipher cmd
            {
                qDebug(LOG_CUSTOM_OVPN) << "Extracted cipher:" << openVpnLine.protocol;
                ovpnData_ += line + "\n";
                if (!openVpnLine.protocol.trimmed().isEmpty())
                    bHasValidCipher = true;
            }
            else
            {
                if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_PROTO) // proto cmd
                {
                    globalProtocol_ = currentProtocol = openVpnLine.protocol;
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted global protocol:" << globalProtocol_;
                }
                else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_PORT) // port cmd
                {
                    globalPort_ = openVpnLine.port;
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted global port:" << globalPort_;
                }
                else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_IGNORE_REDIRECT_GATEWAY)
                {
                    isAllowFirewallAfterConnection_ = false;
                    qDebug(LOG_CUSTOM_OVPN)
                        << "Extracted information: ignore redirect-gateway (" << line << ")";
                }
                else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_DEVICE) // dev cmd
                {
                    if (!openVpnLine.protocol.trimmed().compare("tap", Qt::CaseInsensitive))
                        isTapDevice = true;
                }

                ovpnData_ += line + "\n";
            }
        }

        // Fix protocol in remotes.
        bool hasAnyValidProtocol = false;
        for (auto &r : remotes_) {
            if (r.protocol.isEmpty())
                r.protocol = currentProtocol;
            if (CheckAndFixProtocolName(r.protocol))
                hasAnyValidProtocol = true;
        }

        if (!bFoundVerbCommand)
            ovpnData_ += "verb 3\n";
#ifdef Q_OS_MAC
        // Needed script-security at least 2 on Mac, to allow an "up" command for the DNS setup
        // script.
        if (!bFoundScriptSecurityCommand)
            ovpnData_ += "script-security 2\n";
#endif

        // The "BF-CBC" cipher was the default prior to OpenVPN 2.5.
        // To support old configs that used to work in older Windscribe distributions, add a default
        // cipher command when there is no such command in the config.
        if (!bHasValidCipher)
            ovpnData_ += "cipher BF-CBC\n";

#ifdef Q_OS_MAC
        if (isTapDevice)
        {
            qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << Utils::cleanSensitiveInfo(filepath_) << "uses TAP device, which is not supported on Mac.";
            isCorrect_ = false;
            errMessage_ = "TAP adapter is not supported, please change \"device\" to \"tun\" in the config.";
        }
#endif
        if (!bFoundAtLeastOneRemote)
        {
            qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << Utils::cleanSensitiveInfo(filepath_) << "incorrect, because can't find remote host command. The file format may be incorrect.";
            isCorrect_ = false;
            errMessage_ = "Can't find remote host command";
        }
        else if (!hasAnyValidProtocol)
        {
            qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << Utils::cleanSensitiveInfo(filepath_) << "incorrect, because there is no correct protocol. Please note that IPv6 protocols are not supported.";
            isCorrect_ = false;
            errMessage_ = "Connection protocol is not valid";
        }
        else
        {
            nick_ = remotes_[0].hostname;
        }
    }
    else
    {
        qDebug(LOG_CUSTOM_OVPN) << "Failed to open file" << Utils::cleanSensitiveInfo(filepath_);
        isCorrect_ = false;
        errMessage_ = "Failed to open file";
    }

#ifdef Q_OS_LINUX
#pragma GCC diagnostic pop
#endif
}


} //namespace customconfigs


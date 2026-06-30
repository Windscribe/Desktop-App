#include "parseovpnconfigline.h"

ParseOvpnConfigLine::OpenVpnLine ParseOvpnConfigLine::processLine(const QString &line)
{
    OpenVpnLine openVpnLine;
    openVpnLine.type = OVPN_CMD_UNKNOWN;

    const QString trimmedLine = line.trimmed();
    if (trimmedLine.isEmpty() || trimmedLine.startsWith('#') || trimmedLine.startsWith(';'))
    {
        return openVpnLine;
    }

    const QStringList strs = splitLine(trimmedLine);
    if (strs.isEmpty())
    {
        return openVpnLine;
    }

    const QString &directive = strs[0];
    if (directive.compare("remote", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() >= 2)
        {
            openVpnLine.type = OVPN_CMD_REMOTE_IP;
            openVpnLine.host = strs[1];

            if (strs.count() >= 3)
            {
                openVpnLine.port = strs[2].toUInt();

                if (strs.count() >= 4)
                {
                    openVpnLine.protocol = strs[3].trimmed();
                }
            }
        }
    }
    else if (directive.compare("proto", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() >= 2)
        {
            openVpnLine.type = OVPN_CMD_PROTO;
            openVpnLine.protocol = strs[1].trimmed();
        }
    }
    else if (directive.compare("port", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() >= 2)
        {
            openVpnLine.type = OVPN_CMD_PORT;
            openVpnLine.port = strs[1].toUInt();
        }
    }
    else if (directive.compare("verb", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() >= 2)
        {
            openVpnLine.type = OVPN_CMD_VERB;
            openVpnLine.verb = strs[1].toUInt();
        }
    }
    else if (directive.compare("cipher", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() >= 2)
        {
            openVpnLine.type = OVPN_CMD_CIPHER;
            openVpnLine.protocol = strs[1];
        }
    }
    else if (directive.compare("script-security", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() >= 2)
        {
            openVpnLine.type = OVPN_CMD_SCRIPT_SECURITY;
            openVpnLine.verb = strs[1].toUInt();
        }
    }
    else if (directive.compare("route-nopull", Qt::CaseInsensitive) == 0 ||
             directive.compare("route-noexec", Qt::CaseInsensitive) == 0)
    {
        openVpnLine.type = OVPN_CMD_IGNORE_REDIRECT_GATEWAY;
    }
    else if (directive.compare("pull-filter", Qt::CaseInsensitive) == 0)
    {
        if (strs.count() > 2 &&
            strs[1].compare("ignore", Qt::CaseInsensitive) == 0 &&
            strs[2].compare("redirect-gateway", Qt::CaseInsensitive) == 0)
            openVpnLine.type = OVPN_CMD_IGNORE_REDIRECT_GATEWAY;
    }
    else if (directive.compare("block-outside-dns", Qt::CaseInsensitive) == 0)
    {
        openVpnLine.type = OVPN_CMD_BLOCK_OUTSIDE_DNS;
    }

    return openVpnLine;
}

QStringList ParseOvpnConfigLine::splitLine(const QString &line)
{
    QStringList res;
    int i = 0;
    QString curString;
    bool isInsideString = false;
    bool isInsideQuetes = false;

    while (i < line.size())
    {
        if (isInsideString)
        {
            if (isInsideQuetes)
            {
                curString += line[i];
                if (line[i] == '\'' || line[i] == '\"')
                {
                    isInsideQuetes = false;
                    isInsideString = false;
                    res << curString;
                    curString.clear();
                }
            }
            else
            {
                if (line[i].isSpace())
                {
                    isInsideString = false;
                    res << curString;
                    curString.clear();
                }
                else
                {
                    curString += line[i];
                }
            }
        }
        else
        {
            if (line[i] == '\'' || line[i] == '\"')
            {
                isInsideQuetes = true;
                isInsideString = true;
                curString += line[i];
            }
            else if (!line[i].isSpace())
            {
                isInsideQuetes = false;
                isInsideString = true;
                curString += line[i];
            }
        }
        i++;
    }

    if (!curString.isEmpty())
    {
        res << curString;
    }
    return res;
}

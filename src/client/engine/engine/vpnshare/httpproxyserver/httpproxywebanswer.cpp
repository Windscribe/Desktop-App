#include "httpproxywebanswer.h"
#include "utils/boost_includes.h"
#include "version/appversion.h"

namespace HttpProxyServer {

long HttpProxyWebAnswer::getContentLength()
{
    // Linear scan is fine (few headers). Could use QMap<QString, QString> if this becomes hot.
    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        if (boost::iequals(it->name, "content-length"))
        {
            return atol(it->value.c_str());
        }
    }
    return -1;
}

std::string HttpProxyWebAnswer::processServerHeaders(unsigned int major, unsigned int minor)
{
    std::string ret;
    bool isExistViaHeader = false;
    const std::string via = std::to_string(major) + "." + std::to_string(minor) + " " WS_PRODUCT_NAME " proxy ("
        + AppVersion::instance().version().toStdString() + "/" + AppVersion::instance().build().toStdString() + ")";

    ret = answer + "\r\n";

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        if (!shouldSkipHeader(it->name))
        {
            if (boost::iequals(it->name,"via"))
            {
                ret += "Via: " + it->value + ", " + via + "\r\n";
                isExistViaHeader = true;
            }
            else
            {
                ret += it->name + ": " + it->value + "\r\n";
            }
        }
    }

    if (!isExistViaHeader)
    {
        ret += "Via: " + via + "\r\n";
    }

    // The upstream connection carries a single request, so the client must not reuse this one either.
    ret += "Connection: close\r\n";

    ret += "\r\n";

    return ret;
}

bool HttpProxyWebAnswer::shouldSkipHeader(const std::string &headerName)
{
    if (boost::iequals(headerName,"connection"))
        return true;
    if (boost::iequals(headerName,"proxy-connection"))
        return true;
    if (boost::iequals(headerName,"keep-alive"))
        return true;
    if (boost::iequals(headerName,"proxy-authenticate"))
        return true;
    if (boost::iequals(headerName,"proxy-authorization"))
        return true;

    return false;
}

} // namespace HttpProxyServer

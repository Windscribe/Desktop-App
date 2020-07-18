#include "httpproxywebanswer.h"
#include "Utils/boost_includes.h"
#include "Version/appversion.h"

namespace HttpProxyServer {

long HttpProxyWebAnswer::getContentLength()
{
    //todo: make map instead vector
    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        if (boost::iequals(it->name,"content-length"))
        {
            return atol (it->value.c_str());
        }
    }
    return -1;
}

std::string HttpProxyWebAnswer::processServerHeaders(unsigned int major, unsigned int minor)
{
    std::string ret;
    //todo: check buffer bounds
    char buf[4096];
    bool isExistViaHeader = false;


    ret = answer + "\r\n";


    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        if (!shouldSkipHeader(it->name))
        {
            if (boost::iequals(it->name,"via"))
            {
                int len = sprintf(buf, "Via: %s, %hu.%hu %s (%s/%s)\r\n",
                        it->value.c_str(), (unsigned short)major, (unsigned short)minor, "Windscribe proxy", AppVersion::instance().version().toStdString().c_str(),
                        AppVersion::instance().build().toStdString().c_str());
                Q_ASSERT((unsigned int)len < sizeof(buf));
                ret += buf;
                isExistViaHeader = true;
            }
            else
            {
                ret += it->name + " :" + it->value + "\r\n";
            }
        }
    }

    if (!isExistViaHeader)
    {
        int len = sprintf(buf, "Via: %hu.%hu %s (%s/%s)\r\n",
                (unsigned short)major, (unsigned short)minor, "Windscribe proxy", AppVersion::instance().version().toStdString().c_str(),
                AppVersion::instance().build().toStdString().c_str());
        Q_ASSERT((unsigned int)len < sizeof(buf));
        ret += buf;
    }

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

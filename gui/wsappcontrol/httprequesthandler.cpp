#include "httprequesthandler.h"
#include <fstream>
#include <sstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>

HttpRequestHandler::HttpRequestHandler()
{
}

std::map<std::string, std::string> HttpRequestHandler::handle_request(const HttpRequest &req, HttpReply &rep)
{
    if (!isWindscribeHost(req))
    {
        rep = HttpReply::stock_reply(HttpReply::bad_request);
		return std::map<std::string, std::string>();
    }

	std::map<std::string, std::string> pars = parseParameters(req.content);

    if (!validateParameters(pars))
    {
        rep = HttpReply::stock_reply(HttpReply::bad_request);
		return std::map<std::string, std::string>();
    }

    rep = HttpReply::stock_reply(HttpReply::ok);

    return pars;
}

std::map<std::string, std::string> HttpRequestHandler::parseParameters(const std::string &str)
{
    std::map<std::string, std::string> map;

	std::vector<std::string> pars;
	boost::split(pars, str, boost::is_any_of("&"));

	for (std::vector<std::string>::iterator s = pars.begin(); s != pars.end(); ++s)
    {
		std::vector<std::string> strs;
        std::string s_trimmed = trimmed(*s);
        boost::split(strs, s_trimmed, boost::is_any_of("="));

        if (strs.size() == 2)
        {
			map[trimmed(strs[0])] = trimmed(strs[1]);
        }
        else if (strs.size() == 1)
        {
			map[trimmed(strs[0])] = "";
        }
    }
    return map;
}

bool HttpRequestHandler::validateParameters(const std::map<std::string, std::string> &pars)
{
    if (pars.size() != 3 && pars.size() != 4)
    {
        return false;
    }
	if (pars.find("auth") == pars.end())
    {
        return false;
    }
	if (pars.find("time") == pars.end())
    {
        return false;
    }
	if (pars.find("sig") == pars.end())
    {
        return false;
    }
    if (pars.size() == 4)
    {
		if (pars.find("connect") == pars.end() && pars.find("disconnect") == pars.end())
        {
            return false;
        }
    }
    else if (pars.size() > 4)
    {
        return false;
    }
    return true;
}

bool HttpRequestHandler::isWindscribeHost(const HttpRequest &req)
{
    for (std::vector<HttpHeader>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it)
    {
        std::string name = (*it).name;
        std::string value = (*it).value;
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);
        if (name == "HOST" && (value.find("WSAPPCONTROL.COM") != std::string::npos))
        {
            return true;
        }
    }
    return false;
}

std::string HttpRequestHandler::trimmed(const std::string &s)
{
	std::string str = s;
	boost::trim(str);
	return str;
}

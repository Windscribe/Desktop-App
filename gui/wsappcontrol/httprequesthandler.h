#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H

#include "httpreply.h"
#include "httprequest.h"
#include <map>

class HttpRequestHandler : private boost::asio::noncopyable
{
public:
    HttpRequestHandler();

    std::map<std::string, std::string> handle_request(const HttpRequest& req, HttpReply& rep);

private:
	static std::map<std::string, std::string> parseParameters(const std::string &str);
	static bool validateParameters(const std::map<std::string, std::string> &pars);
    static bool isWindscribeHost(const HttpRequest &req);
	static std::string trimmed(const std::string &s);
};

#endif // HTTPREQUESTHANDLER_H

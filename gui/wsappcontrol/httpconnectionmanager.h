#ifndef HTTPCONNECTIONMANAGER_H
#define HTTPCONNECTIONMANAGER_H

#include <set>
#include <boost/core/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

class HttpConnection;

typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;
typedef bool ServerCallback_T(std::string, bool, bool, std::string);

class HttpConnectionManager : private boost::noncopyable
{
public:
    HttpConnectionManager();

    void start(HttpConnectionPtr c);
    void stop(HttpConnectionPtr c);
    void stop_all();

	size_t count() const;

private:
    std::set<HttpConnectionPtr> connections_;
};

#endif // HTTPCONNECTIONMANAGER_H

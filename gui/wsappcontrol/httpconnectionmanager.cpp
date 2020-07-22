#include "httpconnectionmanager.h"
#include "httpconnection.h"
#include <boost/bind.hpp>

HttpConnectionManager::HttpConnectionManager()
{

}

void HttpConnectionManager::start(HttpConnectionPtr c)
{
    connections_.insert(c);
    c->start();
}

void HttpConnectionManager::stop(HttpConnectionPtr c)
{
	printf("LOG: Client disconnected from the local web server\n");
	fflush(stdout);
    connections_.erase(c);
    c->stop();
}

void HttpConnectionManager::stop_all()
{
    std::for_each(connections_.begin(), connections_.end(), boost::bind(&HttpConnection::stop, _1));
    connections_.clear();
}

size_t HttpConnectionManager::count() const
{
	return connections_.size();
}
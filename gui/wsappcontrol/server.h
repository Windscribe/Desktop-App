#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "httpconnectionmanager.h"
#include "httpconnection.h"
#include "httprequestratelimitcontroller.h"
#include "CertStorage.h"

class Server 
{
public:
	explicit Server(ServerCallback_T *callbackFunc);
	virtual ~Server();

    void start(const std::vector<unsigned int> &ports);

private:
	std::vector<unsigned int> ports_;
	unsigned int port_;
	ServerCallback_T *callbackFunc_;

    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ssl::context context_;

	std::thread *thread_;

	HttpConnectionManager connection_manager_;
	HttpConnectionPtr new_connection_;
	HttpRequestHandler request_handler_;
	HttpRequestRateLimitController httpRequestRateLimitController_;
	CertStorage certStorage_;

	const size_t MAX_NUMBER_OF_CONNECTIONS = 10;

	static void threadFunc(void *t);

	void run();

    void start_accept();
    void handle_accept(const boost::system::error_code& e);
    void handle_stop();
};


#endif // SERVER_H

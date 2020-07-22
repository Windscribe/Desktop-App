#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "httpconnectionmanager.h"
#include "httprequesthandler.h"
#include "httprequest.h"
#include "httpreply.h"
#include "httprequestparser.h"
#include "httprequestratelimitcontroller.h"
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>

class HttpConnection : public boost::enable_shared_from_this<HttpConnection>,
        private boost::noncopyable
{
public:
	HttpConnection(boost::asio::io_service &io_service, boost::asio::ssl::context &context, HttpConnectionManager &manager, HttpRequestHandler &handler,
		HttpRequestRateLimitController &httpRequestRateLimitController, ServerCallback_T *callbackFunc);

	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type &socket();
    void start();
    void stop();

private:
	void handle_handshake(const boost::system::error_code &e);
    void handle_read(const boost::system::error_code &e, std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code &e);
	std::string getSha1(const std::string& p_arg);
	bool isSignatureValid(const std::string &authHash, const std::string &time, const std::string &sig);

	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    HttpConnectionManager &connection_manager_;
    HttpRequestHandler &request_handler_;
    HttpRequestRateLimitController &httpRequestRateLimitController_;
	ServerCallback_T *callbackFunc_;

    boost::array<char, 8192> buffer_;

    HttpRequest request_;
    HttpRequestParser request_parser_;
    HttpReply reply_;
};

#endif // HTTPCONNECTION_H

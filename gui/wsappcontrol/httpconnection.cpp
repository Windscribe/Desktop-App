#include "httpconnection.h"
#include <boost/bind.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <boost/lexical_cast.hpp>

HttpConnection::HttpConnection(boost::asio::io_service& io_service, boost::asio::ssl::context &context, HttpConnectionManager &manager, HttpRequestHandler &handler,
                               HttpRequestRateLimitController &httpRequestRateLimitController,
							   ServerCallback_T *callbackFunc) :
	socket_(io_service, context),
    connection_manager_(manager),
    request_handler_(handler),
    httpRequestRateLimitController_(httpRequestRateLimitController),
	callbackFunc_(callbackFunc),
    request_()
{
    reply_.status = HttpReply::ok;
}

boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type &HttpConnection::socket()
{
	return socket_.lowest_layer();
}

void HttpConnection::start()
{
	socket_.async_handshake(boost::asio::ssl::stream_base::server,
		boost::bind(&HttpConnection::handle_handshake, this,
		boost::asio::placeholders::error));
}

void HttpConnection::stop()
{
    socket().close();
}

void HttpConnection::handle_handshake(const boost::system::error_code &e)
{
	if (!e)
	{
		socket_.async_read_some(boost::asio::buffer(buffer_), boost::bind(&HttpConnection::handle_read, shared_from_this(),
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
	else
	{
		printf("SSL handshake failed\n");
		fflush(stdout);
		connection_manager_.stop(shared_from_this());
	}
}

void HttpConnection::handle_read(const boost::system::error_code &e, size_t bytes_transferred)
{
    if (!e)
    {
        boost::tribool result;
        boost::tie(result, boost::tuples::ignore) = request_parser_.parse(request_, buffer_.data(), buffer_.data() + bytes_transferred);

        if (result)
        {
            if (httpRequestRateLimitController_.isRightRequest())
            {
                //request_.debugToLog();
				std::map<std::string, std::string> params = request_handler_.handle_request(request_, reply_);
                if (reply_.status != HttpReply::bad_request)
                {
                    std::string authHash = params["auth"];

                    if (!isSignatureValid(authHash, params["time"], params["sig"]))
                    {
                        reply_ = HttpReply::stock_reply(HttpReply::bad_request);
                    }
                    else
                    {
						bool bConnectCmd = params.find("connect") != params.end();
						bool bDisconnectCmd = params.find("disconnect") != params.end();
						std::string location;
                        if (bConnectCmd)
                        {
                            location = params["connect"];
                        }
						bool bSuccess = callbackFunc_(authHash, bConnectCmd, bDisconnectCmd, location);

                        reply_.status = HttpReply::ok;
                        if (bSuccess)
                        {
                            reply_.content.append("{\"status\":\"OK\"}");
                        }
                        else
                        {
                            reply_.content.append("{\"status\":\"Failed\"}");
                        }

                        reply_.headers.resize(6);
                        reply_.headers[0].name = "Content-Length";
                        reply_.headers[0].value = boost::lexical_cast<std::string>(reply_.content.size());
                        reply_.headers[1].name = "Content-Type";
                        reply_.headers[1].value = "text/html";

                        // windscribe headers
                        reply_.headers[2].name = "Access-Control-Allow-Credentials";
                        reply_.headers[2].value = "true";

                        reply_.headers[3].name = "Access-Control-Allow-Headers";
                        reply_.headers[3].value = "*";

                        reply_.headers[4].name = "Access-Control-Allow-Methods";
                        reply_.headers[4].value = "POST, GET, OPTIONS";

                        reply_.headers[5].name = "Access-Control-Allow-Origin";
                        reply_.headers[5].value = "https://windscribe.com";
                    }
                }
            }
            else
            {
                reply_ = HttpReply::stock_reply(HttpReply::too_many_requests);
            }

            printf("LOG: Answer sent: %s\n", reply_.toString().c_str());
			fflush(stdout);
            boost::asio::async_write(socket_, reply_.to_buffers(), boost::bind(&HttpConnection::handle_write, shared_from_this(), boost::asio::placeholders::error));
        }
        else if (!result)
        {
            reply_ = HttpReply::stock_reply(HttpReply::bad_request);
            boost::asio::async_write(socket_, reply_.to_buffers(), boost::bind(&HttpConnection::handle_write, shared_from_this(), boost::asio::placeholders::error));
        }
        else
        {
            socket_.async_read_some(boost::asio::buffer(buffer_), boost::bind(&HttpConnection::handle_read, shared_from_this(),
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }
    else if (e != boost::asio::error::operation_aborted)
    {
        connection_manager_.stop(shared_from_this());
    }
}

void HttpConnection::handle_write(const boost::system::error_code &e)
{
    if (!e)
    {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }

    if (e != boost::asio::error::operation_aborted)
    {
        connection_manager_.stop(shared_from_this());
    }
}

std::string HttpConnection::getSha1(const std::string& p_arg)
{
	boost::uuids::detail::sha1 sha1;
	sha1.process_bytes(p_arg.data(), p_arg.size());
	unsigned hash[5] = { 0 };
	sha1.get_digest(hash);

	char buf[41] = { 0 };

	for (int i = 0; i < 5; i++)
	{
		sprintf(buf + (i << 3), "%08x", hash[i]);
	}

	return std::string(buf);
}

bool HttpConnection::isSignatureValid(const std::string &authHash, const std::string &time, const std::string &sig)
{
	std::string str = authHash + time + "canIHAZremoteC0NTROLZ?";
	std::string sha1Hash = getSha1(str);

	std::string sigUpper = sig;
	std::transform(sha1Hash.begin(), sha1Hash.end(), sha1Hash.begin(), ::toupper);
	std::transform(sigUpper.begin(), sigUpper.end(), sigUpper.begin(), ::toupper);

	return sha1Hash == sigUpper;
}

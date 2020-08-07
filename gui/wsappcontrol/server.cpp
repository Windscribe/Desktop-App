#include "Server.h"
#include <boost/bind.hpp>

Server::Server(ServerCallback_T *callbackFunc) :
    port_(0),
	callbackFunc_(callbackFunc),
	io_service_(),
	acceptor_(io_service_),
    context_(boost::asio::ssl::context::sslv23),
	thread_(NULL),
	connection_manager_(),
	new_connection_()
{
	context_.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);
	context_.use_certificate_chain(boost::asio::buffer(certStorage_.getCert()));
	context_.use_private_key(boost::asio::buffer(certStorage_.getPrivateKey()), boost::asio::ssl::context::pem);
}

Server::~Server()
{
	if (thread_)
	{
		boost::system::error_code ec;
		acceptor_.close(ec);
		connection_manager_.stop_all();
		thread_->join();
		delete thread_;
		thread_ = NULL;
		printf("LOG: Local web server stopped on port %u\n", port_);
		fflush(stdout);
	}
}

void Server::start(const std::vector<unsigned int> &ports)
{
	assert(thread_ == NULL);
	assert(ports.size() == 3);
	ports_ = ports;
	thread_ = new std::thread(threadFunc, this);
}


void Server::threadFunc(void *t)
{
	Server *s = static_cast<Server *>(t);
	s->run();
}

void Server::run()
{
	port_ = ports_.front();

	boost::system::error_code ec;

	for (std::vector<unsigned int>::iterator p = ports_.begin(); p != ports_.end(); ++p)
	{
		boost::asio::ip::tcp::resolver resolver(io_service_);
		boost::asio::ip::tcp::resolver::query query("0.0.0.0", std::to_string(*p));

		acceptor_.close(ec);

		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query, ec);
		if (!ec)
		{
			acceptor_.open(endpoint.protocol(), ec);
			if (!ec)
			{
				acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
				if (!ec)
				{
					acceptor_.bind(endpoint, ec);
					if (!ec)
					{
						acceptor_.listen(boost::asio::socket_base::max_connections, ec);
						if (!ec)
						{
							port_ = *p;
							break;
						}
					}
				}
			}
		}
	}

	if (!ec)
	{
		printf("LOG: Local web server started on port %u\n", port_);
		fflush(stdout);
		start_accept();
		io_service_.run();
	}
	else
	{
		printf("LOG: Can't start local web server on ports %u, %u, %u\n", ports_[0], ports_[1], ports_[2]);
		fflush(stdout);
	}
}


void Server::start_accept()
{
	new_connection_.reset(new HttpConnection(io_service_, context_, connection_manager_, request_handler_, httpRequestRateLimitController_, callbackFunc_));
	acceptor_.async_accept(new_connection_->socket(), boost::bind(&Server::handle_accept, this, boost::asio::placeholders::error));
}

void Server::handle_accept(const boost::system::error_code& e)
{
	if (!acceptor_.is_open())
	{
		return;
	}

	if (!e)
	{
		boost::asio::ip::tcp::endpoint remote_ep = new_connection_->socket().remote_endpoint();
		boost::asio::ip::address remote_ad = remote_ep.address();
		std::string strIp = remote_ad.to_string();

		if (strIp != "127.0.0.1")
		{
			printf("LOG: Client rejected, remote host not 127.0.0.1\n");
			connection_manager_.stop(new_connection_);
		}

		if (connection_manager_.count() > MAX_NUMBER_OF_CONNECTIONS)
		{
			printf("LOG: Client rejected, the maximum number of connections exceeded\n");
			connection_manager_.stop(new_connection_);
		}
		else
		{
			printf("LOG: Client connected to local web server on port %u\n", port_);
			fflush(stdout);
			connection_manager_.start(new_connection_);
		}
	}

	start_accept();
}

void Server::handle_stop()
{
	acceptor_.close();
	connection_manager_.stop_all();
}

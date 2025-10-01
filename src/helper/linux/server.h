#pragma once

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

#include <stdio.h>
#include <vector>
#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <list>

#include "routes_manager/routes_manager.h"
#include "wireguard/defaultroutemonitor.h"
#include "wireguard/wireguardadapter.h"
#include "wireguard/wireguardcontroller.h"

typedef boost::shared_ptr<boost::asio::local::stream_protocol::socket> socket_ptr;

class Server
{
public:
    Server();
    ~Server();
    void run();

private:
    boost::asio::io_context io_context_;
    boost::asio::local::stream_protocol::acceptor *acceptor_;

    bool readAndHandleCommand(socket_ptr sock, boost::asio::streambuf *buf, std::string &outCmdAnswer);

    void receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred);
    void acceptHandler(const boost::system::error_code & ec, socket_ptr sock);
    void startAccept();

    bool sendAnswerCmd(socket_ptr sock, const std::string &answer);
    boost::system::error_code safeBlockingWrite(socket_ptr &sock, const void *data, size_t size);
};


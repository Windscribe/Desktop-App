#pragma once

#include <stdio.h>
#include <vector>
#include <thread>
// Avoid warnings from boost library.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#pragma clang diagnostic pop "-Wdeprecated-declarations"
#include <list>

#include "../../posix_common/helper_commands.h"
#include "firewallonboot.h"
#include "macspoofingonboot.h"
#include "split_tunneling/split_tunneling.h"
#include "wireguard/defaultroutemonitor.h"
#include "wireguard/wireguardadapter.h"
#include "wireguard/wireguardcontroller.h"
#include "installer/files.h"
#include "ipv6/ipv6manager.h"

typedef boost::shared_ptr<boost::asio::local::stream_protocol::socket> socket_ptr;

class Server
{
public:
    Server();
    ~Server();
    void run();

private:
    boost::asio::io_service service_;
    boost::asio::local::stream_protocol::acceptor *acceptor_;

    bool readAndHandleCommand(socket_ptr sock, boost::asio::streambuf *buf, CMD_ANSWER &outCmdAnswer);

    void receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred);
    void acceptHandler(const boost::system::error_code & ec, socket_ptr sock);
    void startAccept();
    void runService();

    bool sendAnswerCmd(socket_ptr sock, const CMD_ANSWER &cmdAnswer);
};

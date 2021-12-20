
#ifndef ____Server__
#define ____Server__

#include <stdio.h>
#include <vector>
#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <list>

#include "../../posix_common/helper_commands.h"
#include "wireguard/wireguardadapter.h"
#include "wireguard/wireguardcontroller.h"


typedef boost::shared_ptr<boost::asio::local::stream_protocol::socket> socket_ptr;

class Server
{
public:
    Server();
    ~Server();
    void run();
    void stop();
  
private:
    //SplitTunneling splitTunneling_;
    WireGuardController wireGuardController_;
    boost::asio::io_service service_;
    boost::asio::local::stream_protocol::acceptor *acceptor_;
    
    //Files *files_;
   
    bool readAndHandleCommand(socket_ptr sock, boost::asio::streambuf *buf, CMD_ANSWER &outCmdAnswer);
    
    void receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred);
    void acceptHandler(const boost::system::error_code & ec, socket_ptr sock);
    void startAccept();
    void runService();
    
    bool sendAnswerCmd(socket_ptr sock, const CMD_ANSWER &cmdAnswer);
};

#endif /* defined(____Server__) */

#pragma once
// #include <boost/beast/http.hpp>
// #include <boost/beast.hpp>
// #include <boost/asio.hpp>
#include "global.h"
#include "CSession.h"

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class CSession;

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, unsigned short& port);
    ~CServer();
    void ClearSession(std::string uuid);
private:
    void HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code & error);
    void StartAccept();
    boost::asio::io_context& _io_context;
    short _port;
    tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
};

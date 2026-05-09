#pragma once

#include "global.h"

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& io_context, short& port);
    ~CServer();
    void ClearSession();
private:
    void HandleAccept();
    void StartAccept();
    boost::asio::io_context& m_context;
    short m_port;
    tcp::acceptor m_acceptor;
    std::map<std::string, std::shared_ptr<>> m_sessions;
    std::mutex m_mutex;
};
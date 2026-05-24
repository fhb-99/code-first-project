#include "CServer.h"
#include "CSession.h"
#include "AsioIOServicePool.h"
#include <functional>
#include <iostream>

CServer::CServer(boost::asio::io_context& io_context, unsigned short& port)
    : m_context(io_context),
      m_port(port),
      m_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
    std::cout << "Server start success, listen on port: " << m_port << std::endl;
    StartAccept();
}

CServer::~CServer()
{
    std::cout << "Server destruct, listen on port: " << m_port << std::endl;
}

void CServer::StartAccept()
{
    boost::asio::io_context& worker_io = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(worker_io, this);
    m_acceptor.async_accept(new_session->GetSocket(),
        std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::ClearSession(std::string session_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(session_id);
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->Start();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.emplace(new_session->GetSessionId(), new_session);
    }
    else
    {
        std::cout << "session accept failed, error is " << error.message() << std::endl;
    }

    StartAccept();
}

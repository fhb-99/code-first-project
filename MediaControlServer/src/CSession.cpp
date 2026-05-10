#include "CSession.h"
#include "LogicSystem.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

CSession::CSession(boost::asio::io_context& io_context, CServer * server)
    : m_socket(io_context),
      m_server(server),
      m_parse_head(false)
{
    boost::uuids::uuid uid = boost::uuids::random_generator()();
    m_session_id = static_cast<int>(boost::uuids::to_string(uid));
    m_msg_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN, 0);
}


CSession::~CSession()
{
    std::cout << "CSession destruct, session id is " << m_session_id << std::endl;
}

tcp::socket& CSession::GetSocket()
{
    return m_socket;
}

std::string& CSession::GetSessionId()
{
    return m_session_id;
}

void CSession::Start()
{
    AsyncReadHead(HEAD_TOTAL_LEN);
}


std::shared_ptr<CSession> CSession::SharedSelf()
{
    return std::shared_from_this();
}


void CSession::Close()
{
    m_socket.close();
    m_server->ClearSession(m_session_id);
}

void CSession::AsyncReadHead(int length)
{
    auto self = std::shared_from_this();
    asyncReadFull(length, [self, this](boost::system::error_code& error, std::size_t bytes_transfered){
        try
        {
            if(error)
            {
                std::cout << "handle read failed, error is " << error.message() << std::endl;
                Close();
                m_server->ClearSession(m_session_id);
                return;
            }

            if(bytes_transfered < length)
            {
                std::cout << "handle read failed, error is " << error.message() << std::endl;
                Close();
                m_server->ClearSession(m_session_id);
                return;
            }

            m_msg_node->clear();
            //长度够了头部四字节
            memcpy(m_msg_node->data, m_data, bytes_transfered);

            //获取id
            short msg_id;
            memcpy(&msg_id, m_msg_node->data, HEAD_ID_LEN);
            //转本地字节序
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
            std::cout << "msg_id is " << msg_id << std::endl;
            //id非法
            if (msg_id > MAX_LENGTH) 
            {
                std::cout << "invalid msg_id is " << msg_id << std::endl;
                m_server->ClearSession(m_session_id);
                return;
            }

            //获取消息体长度
            short msg_len;
            memcpy(&msg_len, m_msg_node->data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
            std::cout << "msg_len is " << msg_len << std::endl;
            //长度非法
            if (msg_len > MAX_LENGTH) 
            {
                std::cout << "invalid msg_len is " << msg_len << std::endl;
                m_server->ClearSession(m_session_id);
                return;
            }

            m_recv_node = std::make_shared<RecvNode>(msg_len, msg_id);
            AsyncReadBody(msg_len);
        }
        catch (std::exception& e) 
        {
            std::cout << "Exception code is " << e.what() << std::endl;
        }
    });
}


void CSession::AsyncReadBody(int length)
{
    auto self = std::shared_from_this();
    asyncReadFull(length, [self, this](boost::system::error_code& error, std::size_t bytes_transfered){
        try 
        {
            if (error) 
            {
                std::cout << "handle read failed, error is " << error.message() << std::endl;
                Close();
                m_server->ClearSession(m_session_id);
                return;
            }

            if (bytes_transfered < length) 
            {
                std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
                    << length<<"]" << std::endl;
                Close();
                m_server->ClearSession(m_session_id);
                return;
            }

            memcpy(m_recv_node->data , m_data , bytes_transfered);
            m_recv_node->m_cur_len += bytes_transfered;
            m_recv_node->data[m_recv_node->m_total_len] = '\0';
            std::cout << "receive data is " << m_recv_node->data << std::endl;
            LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(self, m_recv_node));
            AsyncReadHead(HEAD_TOTAL_LEN);
        }
        catch (std::exception& e) 
        {
            std::cout << "Exception code is " << e.what() << std::endl;
        }
    });
}


void CSession::asyncReadFull(std::size_t length, std::function<void(boost::system::error_code& error, std::size_t bytes_transfered)>handler)
{
    memset(m_data, 0, MAX_LENGTH);
    asyncReadLen(0, length, handler);
}


void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(boost::system::error_code& error, std::size_t bytes_transfered)>handler)
{
    auto self = std::shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_data + read_len, total_len - read_len), 
    [read_len, total_len, handler, self](boost::system::error_code& error, std::size_t bytes_transfered){
        if(error) 
        {
            //出现错误，直接执行回调
            handler(error, bytes_transfered + read_len);
            return;
        }
        if(bytes_transfered + read_len >= total_len)
        {
            //长度够了，也执行回调
            handler(error, bytes_transfered + read_len);
            return;
        }
        self->asyncReadLen(read_len + bytes_transfered, total_len, handler);
    });
}


void Send(char* msg,  short max_length, short msgid)
{
    std::lock_guard<std::mutex> lock(send_mutex);
    int send_queue_size = send_que.size();
    if(send_queue_size > MAX_SENDQUE)
    {
        std::cout << "session: " << m_session_id << " send queue failed, size is: " << MAX_SENDQUE << std::endl;
        return;
    }

    send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));

    if(send_queue_size > 0)
    {
        return;
    }

    auto& msgnode = send_que.front();
    boost::asio::async_write(m_socket, boost::asio::buffer(msgnode->data, msgnode->m_total_len),
        std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}


void Send(std::string msg, short msgid)
{
    std::lock_guard<std::mutex> lock(send_mutex);
    int send_queue_size = send_que.size();
    if(send_queue_size > MAX_SENDQUE)
    {
        std::cout << "session: " << m_session_id << " send queue failed, size is: " << MAX_SENDQUE << std::endl;
        return;
    }
    
    send_que.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));

    if(send_queue_size > 0)
    {
        return;
    }

    auto& msgnode = send_que.front();
    boost::asio::async_write(m_socket, boost::asio::buffer(msgnode->data, msgnode->m_total_len),
        std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}


void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self)
{
    try
    {   
        if(!error)
        {
            std::lock_guard<std::mutex> lock(send_mutex);
            send_que.pop();
            if(!send_que.empty())
            {
                auto& msgnode = send_que.front();
                boost::asio::async_write(m_socket, boost::asio::buffer(msgnode->data, msgnode->m_total_len),
                    std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
            }
        }
        else
        {
            std::cout << "handle write failed, error is : " << error.message() << std::endl;
            Close();
            m_server->ClearSession(m_session_id);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}




LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
    : _session(std::move(session)),
      _recvnode(std::move(recvnode))
{

}
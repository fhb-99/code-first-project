#pragma once

#include "global.h"
#include "CServer.h"
#include "MsgNode.h"

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, CServer * server);
    ~CSession();

    void start();
    void close();

    void AsyncReadHead(int length);
    void AsyncReadBody(int length);
private:
    void asyncReadFull(std::size_t length, 
            std::function<void(boost::system::error_code& error, std::size_t bytes_transfered)>handler);
    void asyncReadLen(std::size_t read_len, std::size_t total_len, 
            std::function<void(boost::system::error_code& error, std::size_t bytes_transfered)>handler);


    tcp::socket m_socket;
    std::string m_session_id;  //每个处理会话都有一个唯一的id
    char m_data[MAX_LENGTH];
    CServer * m_server;
    std::mutex send_mutex;
    bool m_parse_head;
    std::shared_ptr<MsgNode> m_msg_node; // 中转
    std::shared_ptr<RecvNode> m_recv_node; //存放读取后的消息体的长度和id

};
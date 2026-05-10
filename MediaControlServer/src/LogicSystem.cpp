#include "LogicSystem.h"

#include <iostream>



LogicSystem::LogicSystem()
    : _b_stop(false)
{
    RegisterCallBacks();
    _worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _b_stop = true;
    }
    _consume.notify_all();
    if (_worker_thread.joinable())
    {
        _worker_thread.join();
    }
}

void LogicSystem::RegisterCallBacks()
{
    // TODO(MediaControlServer): 按业务协议定义 short msg_id 常量后，在此注册 FunCallBack。
    // 示例：_fun_callbacks[MSG_MEDIA_XXX] = std::bind(&LogicSystem::SomeHandler, this, ...);
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
    std::unique_lock<std::mutex> unique_lock(_mutex);
    _msg_que.push(std::move(msg));
    if (_msg_que.size() == 1)
    {
        unique_lock.unlock();
        _consume.notify_one();
    }
}

void LogicSystem::DealMsg()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_msg_que.empty() && !_b_stop)
        {
            _consume.wait(lock);
        }

        if (_b_stop)
        {
            while (!_msg_que.empty())
            {
                auto msg_node = _msg_que.front();
                std::cout << "[LogicSystem] shutdown drain, msg_id=" << msg_node->_recvnode->m_msg_id << std::endl;
                auto it = _fun_callbacks.find(msg_node->_recvnode->m_msg_id);
                if (it != _fun_callbacks.end())
                {
                    it->second(msg_node->_session, msg_node->_recvnode->m_msg_id,
                        std::string(msg_node->_recvnode->data, msg_node->_recvnode->m_cur_len));
                }
                _msg_que.pop();
            }
            break;
        }

        auto msg_node = _msg_que.front();
        const short msg_id = msg_node->_recvnode->m_msg_id;
        std::cout << "[LogicSystem] recv msg_id=" << msg_id << std::endl;

        auto it = _fun_callbacks.find(msg_id);
        if (it == _fun_callbacks.end())
        {
            _msg_que.pop();
            std::cout << "[LogicSystem] msg_id [" << msg_id << "] handler not registered (stub)" << std::endl;
            continue;
        }

        it->second(msg_node->_session, msg_id,
            std::string(msg_node->_recvnode->data, msg_node->_recvnode->m_cur_len));
        _msg_que.pop();
    }
}

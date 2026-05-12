#include "LogicSystem.h"
#include "MysqlMgr.h"

#include <functional>
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
    _fun_callbacks[ID_MEDIA_LIST_REQ] = std::bind(&LogicSystem::MediaListHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[ID_MEDIA_PLAY_REQ] = std::bind(&LogicSystem::MediaPlayHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[ID_MEDIA_STOP_REQ] = std::bind(&LogicSystem::MediaStopHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[ID_MEDIA_PAUSE_REQ] = std::bind(&LogicSystem::MediaPauseHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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



void LogicSystem::MediaListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_LIST_RSP);
        return;
    }

    const int uid = root["uid"].asInt();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_LIST_RSP);
    });

    std::vector<std::shared_ptr<MediaListInfo>> media_list;
    const bool success = MysqlMgr::GetInstance()->GetMediaList(uid, media_list);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    for (auto& media : media_list)
    {
        Json::Value media_info;
        media_info["id"] = media->id;
        media_info["stream_id"] = media->stream_id;
        media_info["name"] = media->name;
        media_info["url"] = media->url;
        media_info["source_type"] = media->source_type;
        media_info["status"] = media->status;
        media_info["owner_id"] = media->owner_id;
        rtvalue["media_list"].append(media_info);
    }
}



void LogicSystem::MediaPlayHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    Josn::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_PLAY_RSP);
        return;
    }

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_PLAY_RSP);
    });

    //根据客户端传来的要播放的url，uid以及stream_id，
    // 在数据库中查询session_id(暂时读取房间名（也就是sessin_id）），还有对应的session_name,
    // 暂时先这样，后续再优化
    const int uid = root["uid"].asInt();
    const std::string url = root["url"].asString();
    const std::string stream_id = root["stream_id"].asString();

    std::string session_id;
    std::string session_name;
    bool success = MysqlMgr::GetInstance()->GetSessionInfo(uid, session_id, session_name);
    if (!success)
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_PLAY_RSP);
        return;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["session_id"] = session_id;
    rtvalue["session_name"] = session_name;
    
}


void LogicSystem::MediaStopHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{

}

void LogicSystem::MediaPauseHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{

}
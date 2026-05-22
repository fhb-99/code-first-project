#include "LogicSystem.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"

#include <functional>
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>



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

    //session请求处理
    _fun_callbacks[ID_MEDIA_SESSION_LIST_REQ] = std::bind(&LogicSystem::MediaSessionListHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callbacks[ID_MEDIA_CREATE_SESSION_REQ] = std::bind(&LogicSystem::MediaCreateSessionHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callbacks[ID_MEDIA_JOIN_SESSION_REQ] = std::bind(&LogicSystem::MediaJoinSessionHandler, this,
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
    (void)msg_id;
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

    //现在测试，只查数据库
    // Hash key：media_play_info<uid>；列表快速路径读取字段 _last_url（与 MediaPlayHandler 写入一致）
    /*
    const std::string redisKey = "media_play_info" + std::to_string(uid);
    const std::string media_play_info = RedisMgr::GetInstance()->HGet(redisKey, "_last_url");
    if (!media_play_info.empty())
    {
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["media_play_info"] = media_play_info;
        return;
    }
    else
    {
        //如果redis当中不存在，则查询数据库
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
            rtvalue["media_list"].append(media_info);
        }
    }  
    */
    
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
            rtvalue["media_list"].append(media_info);
        }
}

void LogicSystem::MediaPlayHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_PLAY_RSP);
        return;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_PLAY_RSP);
    });

    //根据客户端传来的要播放的url，uid以及stream_id，
    // 在数据库中查询session_id；迁移脚本无 session 表，来自 media_client_playing（无记录则失败，需先写入播放会话）
    const int uid = root["uid"].asInt();
    const std::string url = root["url"].asString();
    const std::string stream_id = root["stream_id"].asString();
    std::string session_id = root["session_id"].asString();
    
    //如果客户端传来的session_id为空时，则默认
    if (session_id.empty())
    {
        session_id = "default";
    }

    //打印播放请求中的信息
    std::cout << "uid: " << uid << "  url: " << url << 
        "  stream_id:  " << stream_id << "  session_id:  " << session_id << std::endl;
    
    //更新media_session_stream表与media_client_playing表
    bool success = MysqlMgr::GetInstance()->InsertMediaSessionStream(uid, session_id, stream_id, 1);
    bool success2 = MysqlMgr::GetInstance()->InsertMediaClientPlaying(uid, session_id, stream_id, 1);
    if (!success || !success2)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    
    //更新完后，在线人数加一
    bool success3 = MysqlMgr::GetInstance()->UpdateMediaSessionOnlineCount(uid, session_id, stream_id, true);
    if (!success3)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    //存入到redis当中（主字段为 session_id；_last_url 供媒体列表快速路径）
    const std::string redisKey = "media_play_info" + std::to_string(uid);
    bool flag = RedisMgr::GetInstance()->HSet(redisKey, session_id, url);
    if (flag)
    {
        (void)RedisMgr::GetInstance()->HSet(redisKey, "_last_url", url);
    }
    if (!flag)
    {
        rtvalue["error"] = ErrorCodes::Error_Redis;
        return;
    }

    rtvalue["error"] = ErrorCodes::Success;
    // 回包给客户端的播放信息
    rtvalue["stream_id"] = stream_id;
    rtvalue["session_id"] = session_id;
    rtvalue["play_url"] = url;
}


void LogicSystem::MediaStopHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_STOP_RSP);
        return;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_STOP_RSP);
    });

    const int uid = root["uid"].asInt();
    const std::string session_id = root["session_id"].asString();
    const std::string stream_id = root["stream_id"].asString();

    //更新media_session_stream表与media_client_playing表
    bool success = MysqlMgr::GetInstance()->InsertMediaSessionStream(uid, session_id, stream_id, 0);
    bool success2 = MysqlMgr::GetInstance()->InsertMediaClientPlaying(uid, session_id, stream_id, 0);
    if (!success || !success2)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    //更新完后，在线人数减一
    bool success3 = MysqlMgr::GetInstance()->UpdateMediaSessionOnlineCount(uid, session_id, stream_id, false);
    if (!success3)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    //从 redis 删除会话字段与列表缓存字段（失败不阻断，DB 已更新）
    const std::string redisKey = "media_play_info" + std::to_string(uid);
    (void)RedisMgr::GetInstance()->HDel(redisKey, session_id);
    (void)RedisMgr::GetInstance()->HDel(redisKey, "_last_url");

    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["stream_id"] = stream_id;
    rtvalue["session_id"] = session_id;
}

void LogicSystem::MediaPauseHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    (void)session;
    (void)msg_id;
    (void)msg_data;
    //好像业务上不需要
}



void LogicSystem::MediaSessionListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    // 表职责提示：
    // - media_session：会话元信息/当前状态（session 列表、当前播放流、状态）
    // - media_session_stream：会话与流的绑定 + 在线人数统计
    // 会话列表请求仅查 media_session。
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_SESSION_LIST_RSP);
    });

    const int uid = root["uid"].asInt();

    std::vector<std::shared_ptr<SessionInfo>> session_list;
    const bool success = MysqlMgr::GetInstance()->GetSessionList(uid, session_list);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    for (auto& session : session_list)
    {
        Json::Value session_info;
        session_info["session_id"] = session->session_id;
        session_info["owner_id"] = session->owner_id;
        session_info["current_stream_id"] = session->current_stream_id;
        session_info["state"] = session->state;
        rtvalue["sessions"].append(session_info);
    }
}


void LogicSystem::MediaCreateSessionHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    // 创建会话：写 media_session（会话元信息），并初始化 media_session_stream 绑定流。
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_CREATE_SESSION_RSP);
    });

    const int uid = root["uid"].asInt();
    const std::string stream_id = root["stream_id"].asString();
    if (stream_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_StreamID;
        return;
    }

    const bool flag = MysqlMgr::GetInstance()->IsStreamIDValid(stream_id);
    if (!flag)
    {
        rtvalue["error"] = ErrorCodes::Error_StreamID;
        rtvalue["stream_id"] = stream_id;
        return;
    }

    const std::string session_id = boost::uuids::to_string(boost::uuids::random_generator()());
    const bool success = MysqlMgr::GetInstance()->CreateSession(uid, session_id, stream_id, 1);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::Error_Mysql;
        return;
    }

    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["session_id"] = session_id;
    rtvalue["stream_id"] = stream_id;
}


void LogicSystem::MediaJoinSessionHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    // 加入会话：不改 media_session，仅记录成员状态（当前先落到 media_client_playing）。
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_JOIN_SESSION_RSP);
    });
    
    const int uid = root["uid"].asInt();
    const std::string session_id = root["session_id"].asString();
    const std::string stream_id = root["stream_id"].asString();
    if (session_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (stream_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_StreamID;
        return;
    }

    const int owner_id = MysqlMgr::GetInstance()->GetOwnerIDOfSession(session_id);
    if (owner_id == 0)
    {
        rtvalue["error"] = ErrorCodes::Error_Mysql;
        return;
    }

    const bool flag = MysqlMgr::GetInstance()->IsStreamIDValid(stream_id);
    if (!flag)
    {
        rtvalue["error"] = ErrorCodes::Error_StreamID;
        rtvalue["stream_id"] = stream_id;
        return;
    }

    const bool success = MysqlMgr::GetInstance()->JoinSession(uid, session_id, stream_id, 0);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::Error_Mysql;
        return;
    }

    rtvalue["session_id"] = session_id;
    rtvalue["stream_id"] = stream_id;
}
#include "LogicSystem.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"

#include <functional>
#include <iostream>
#include <chrono>
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

    // session 请求处理
    _fun_callbacks[ID_MEDIA_SESSION_LIST_REQ] = std::bind(&LogicSystem::MediaSessionListHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callbacks[ID_MEDIA_CREATE_SESSION_REQ] = std::bind(&LogicSystem::MediaCreateSessionHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callbacks[ID_MEDIA_JOIN_SESSION_REQ] = std::bind(&LogicSystem::MediaJoinSessionHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 心跳请求处理
    _fun_callbacks[ID_MEDIA_HEARTBEAT_REQ] = std::bind(&LogicSystem::MediaHeartbeatHandler, this,
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


// ============================================================================
// 媒体列表请求处理
// ============================================================================
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
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);

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
        rtvalue["media_list"].append(media_info);
    }
}


// ============================================================================
// 播放请求处理 — 仅 session owner 有权控制播放，操作后广播同步通知
// ============================================================================
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

    const int uid = root["uid"].asInt();
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);
    const std::string url = root["url"].asString();
    const std::string stream_id = root["stream_id"].asString();
    std::string session_id = root["session_id"].asString();

    // 未指定会话时走 default（单人模式，不广播）
    if (session_id.empty())
    {
        session_id = "default";
    }

    std::cout << "[MediaPlay] uid: " << uid << " url: " << url
              << " stream_id: " << stream_id << " session_id: " << session_id << std::endl;

    // 非 default 会话：校验 owner 权限
    if (session_id != "default")
    {
        if (!IsSessionOwner(uid, session_id))
        {
            rtvalue["error"] = ErrorCodes::Error_NotOwner;
            std::cout << "[MediaPlay] uid " << uid << " is not owner of session " << session_id << std::endl;
            return;
        }
    }

    // 更新 media_session_stream 表与 media_client_playing 表
    bool success = MysqlMgr::GetInstance()->InsertMediaSessionStream(uid, session_id, stream_id, 1);
    bool success2 = MysqlMgr::GetInstance()->InsertMediaClientPlaying(uid, session_id, stream_id, 1);
    if (!success || !success2)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // 在线人数加一
    bool success3 = MysqlMgr::GetInstance()->UpdateMediaSessionOnlineCount(uid, session_id, stream_id, true);
    if (!success3)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // 更新 media_session 播放状态（position_ms 从 0 开始）
    if (session_id != "default")
    {
        MysqlMgr::GetInstance()->UpdateSessionPlayState(session_id, stream_id, 1, 0);
    }

    // 存入 redis
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

    // 非 default 会话：向所有成员广播同步通知
    if (session_id != "default")
    {
        BroadcastSyncNotify(session_id, stream_id, "play", 0, uid);
    }

    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["stream_id"] = stream_id;
    rtvalue["session_id"] = session_id;
    rtvalue["play_url"] = url;
}


// ============================================================================
// 停止播放处理
// ============================================================================
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
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);
    const std::string session_id = root["session_id"].asString();
    const std::string stream_id = root["stream_id"].asString();

	// 拒绝空 session_id：合法值只能是 "default" 或真实会话 ID
	if (session_id.empty())
	{
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

    // 非 default 会话：校验 owner 权限
    if (session_id != "default" && !session_id.empty())
    {
        if (!IsSessionOwner(uid, session_id))
        {
            rtvalue["error"] = ErrorCodes::Error_NotOwner;
            std::cout << "[MediaStop] uid " << uid << " is not owner of session " << session_id << std::endl;
            return;
        }
    }

    // 更新状态为 stopped(0)
    bool success = MysqlMgr::GetInstance()->InsertMediaSessionStream(uid, session_id, stream_id, 0);
    bool success2 = MysqlMgr::GetInstance()->InsertMediaClientPlaying(uid, session_id, stream_id, 0);
    if (!success || !success2)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // 在线人数减一
    bool success3 = MysqlMgr::GetInstance()->UpdateMediaSessionOnlineCount(uid, session_id, stream_id, false);
    if (!success3)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // 更新 media_session 播放状态
    if (session_id != "default" && !session_id.empty())
    {
        MysqlMgr::GetInstance()->UpdateSessionPlayState(session_id, stream_id, 0, 0);
    }

    // 从 redis 删除会话字段
    const std::string redisKey = "media_play_info" + std::to_string(uid);
    (void)RedisMgr::GetInstance()->HDel(redisKey, session_id);
    (void)RedisMgr::GetInstance()->HDel(redisKey, "_last_url");

    // 非 default 会话：广播同步通知
    if (session_id != "default" && !session_id.empty())
    {
        BroadcastSyncNotify(session_id, stream_id, "stop", 0, uid);
    }

    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["stream_id"] = stream_id;
    rtvalue["session_id"] = session_id;
}


// ============================================================================
// 暂停播放处理 — 仅 owner 可操作，暂停后广播同步通知
// ============================================================================
void LogicSystem::MediaPauseHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_PAUSE_RSP);
        return;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_MEDIA_PAUSE_RSP);
    });

    const int uid = root["uid"].asInt();
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);
    const std::string session_id = root["session_id"].asString();
    const std::string stream_id = root["stream_id"].asString();
    const long long pos_ms = root.get("position_ms", 0).asInt64();
	// paused=true 表示暂停, false 表示恢复播放
	const bool paused = root.get("paused", true).asBool();
	const int target_state = paused ? 2 : 1;
	const std::string action = paused ? "pause" : "play";

    // 非 default 会话：校验 owner 权限
    if (session_id != "default" && !session_id.empty())
    {
        if (!IsSessionOwner(uid, session_id))
        {
            rtvalue["error"] = ErrorCodes::Error_NotOwner;
            std::cout << "[MediaPause] uid " << uid << " is not owner of session " << session_id << std::endl;
            return;
        }
    }

    // 更新播放状态为 paused(2)，记录当前位置
    bool success = MysqlMgr::GetInstance()->UpdateMediaPlayStatus(uid, session_id, stream_id, target_state);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::Error_Mysql;
        return;
    }

    if (session_id != "default" && !session_id.empty())
    {
        MysqlMgr::GetInstance()->UpdateSessionPlayState(session_id, stream_id, target_state, pos_ms);
        BroadcastSyncNotify(session_id, stream_id, action, pos_ms, uid);
    }

    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["session_id"] = session_id;
    rtvalue["stream_id"] = stream_id;
    rtvalue["position_ms"] = static_cast<Json::Int64>(pos_ms);
}


// ============================================================================
// 会话列表请求处理
// ============================================================================
void LogicSystem::MediaSessionListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
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
    for (auto& s : session_list)
    {
        Json::Value item;
        item["session_id"] = s->session_id;
        item["session_name"] = s->session_name;
        item["owner_id"] = s->owner_id;
        item["current_stream_id"] = s->current_stream_id;
        item["current_pos_ms"] = static_cast<Json::Int64>(s->current_pos_ms);
        item["sync_version"] = s->sync_version;
        item["state"] = s->state;
        rtvalue["sessions"].append(item);
    }
}


// ============================================================================
// 创建会话处理 — 写入 media_session 并将会话创建者加入成员表
// ============================================================================
void LogicSystem::MediaCreateSessionHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
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
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);
    const std::string stream_id = root["stream_id"].asString();
    const std::string session_name = root.get("session_name", "").asString();
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
    const bool success = MysqlMgr::GetInstance()->CreateSession(uid, session_id, session_name, stream_id, 1);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::Error_Mysql;
        return;
    }

    // 将会话创建者加入成员表
    MysqlMgr::GetInstance()->AddSessionMember(uid, session_id);

    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["session_id"] = session_id;
    rtvalue["stream_id"] = stream_id;
}


// ============================================================================
// 加入会话处理 — 写入成员表并返回当前会话播放状态，便于客户端同步
// ============================================================================
void LogicSystem::MediaJoinSessionHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
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
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);
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
        rtvalue["error"] = ErrorCodes::Error_SessionNotFound;
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

    // 加入成员表并增加在线人数
    MysqlMgr::GetInstance()->AddSessionMember(uid, session_id);
    MysqlMgr::GetInstance()->UpdateMediaSessionOnlineCount(uid, session_id, stream_id, true);

    // 回包附带当前会话播放状态，便于客户端同步到最新位置
    std::vector<std::shared_ptr<SessionInfo>> session_list;
    if (MysqlMgr::GetInstance()->GetSessionList(uid, session_list))
    {
        for (auto& s : session_list)
        {
            if (s->session_id == session_id)
            {
                rtvalue["current_stream_id"] = s->current_stream_id;
                rtvalue["current_pos_ms"] = static_cast<Json::Int64>(s->current_pos_ms);
                rtvalue["state"] = s->state;
                break;
            }
        }
    }

    rtvalue["session_id"] = session_id;
    rtvalue["stream_id"] = stream_id;
}


// ============================================================================
// 心跳请求处理 — 更新成员最近心跳时间，用于在线状态管理
// ============================================================================
void LogicSystem::MediaHeartbeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        Json::Value err;
        err["error"] = ErrorCodes::Error_Json;
        session->Send(err.toStyledString(), ID_MEDIA_HEARTBEAT_RSP);
        return;
    }

    const int uid = root["uid"].asInt();
	// 将 uid 与当前 TCP 连接绑定，供广播同步通知时查找目标连接
	UserMgr::GetInstance()->SetUserSession(uid, session);
    const std::string session_id = root["session_id"].asString();

    if (!session_id.empty())
    {
        MysqlMgr::GetInstance()->UpdateMemberHeartbeat(uid, session_id);
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    session->Send(rtvalue.toStyledString(), ID_MEDIA_HEARTBEAT_RSP);
}


// ============================================================================
// 辅助方法：判断 uid 是否为指定会话的 owner
// ============================================================================
bool LogicSystem::IsSessionOwner(int uid, const std::string& session_id)
{
    const int owner_id = MysqlMgr::GetInstance()->GetOwnerIDOfSession(session_id);
    return owner_id == uid;
}


// ============================================================================
// 辅助方法：向会话内所有成员广播同步通知
// 同步通知格式（与客户端 StreamController::slot_on_media_sync_notify 对齐）：
// {
//   "session_id": "...",
//   "stream_id": "...",
	//   "play_url": "file:///...",
//   "action": "play|pause|seek|stop",
//   "position_ms": 123456,
//   "server_ts_ms": 1710000000000,
//   "operator_uid": 10001
// }
// ============================================================================
void LogicSystem::BroadcastSyncNotify(const std::string& session_id, const std::string& stream_id,
                                       const std::string& action, long long position_ms, int operator_uid)
{
    // 查询会话内所有成员
    std::vector<std::shared_ptr<SessionMemberInfo>> members;
    if (!MysqlMgr::GetInstance()->GetSessionMembers(session_id, members))
    {
        std::cout << "[BroadcastSync] failed to get members for session " << session_id << std::endl;
        return;
    }

    if (members.empty())
    {
        std::cout << "[BroadcastSync] no members in session " << session_id << std::endl;
        return;
    }

    // 构造同步通知 JSON
    Json::Value notify;

	// 根据 stream_id 查询播放 URL，填充 play_url 字段供客户端直接播放
	std::string play_url;
	if (!MysqlMgr::GetInstance()->GetStreamUrl(stream_id, play_url))
	{
		std::cout << "[BroadcastSync] failed to get url for stream_id=" << stream_id << std::endl;
		// 查询失败不阻断广播，客户端可根据 stream_id 自行匹配 URL
	}

    notify["session_id"] = session_id;
    notify["stream_id"] = stream_id;
	notify["play_url"] = play_url;
    notify["action"] = action;
    notify["position_ms"] = static_cast<Json::Int64>(position_ms);
    notify["server_ts_ms"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    notify["operator_uid"] = operator_uid;

    const std::string notify_str = notify.toStyledString();

    // 向每个在线成员推送同步通知
    for (auto& member : members)
    {
        // 跳过操作者本人（操作者已通过 RSP 消息确认，无需重复通知）
        if (member->uid == operator_uid)
        {
            continue;
        }

        auto member_session = UserMgr::GetInstance()->GetSession(member->uid);
        if (member_session)
        {
            std::cout << "[BroadcastSync] push " << action << " to uid=" << member->uid
                      << " for session " << session_id << std::endl;
            member_session->Send(notify_str, ID_MEDIA_SYNC_NOTIFY);
        }
    }
}

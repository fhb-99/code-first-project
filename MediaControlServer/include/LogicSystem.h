#pragma once

#include <thread>

#include "global.h"
#include "Singleton.h"
#include "CSession.h"
#include "MsgNode.h"
#include "data.h"

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();

    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
    LogicSystem();

    void DealMsg();
    void RegisterCallBacks();

    void MediaListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaPlayHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaStopHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaPauseHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaSessionListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaCreateSessionHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaJoinSessionHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void MediaHeartbeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);

    // 判断 uid 是否为指定会话的 owner（只有 owner 能执行播放控制操作）
    bool IsSessionOwner(int uid, const std::string& session_id);
    // 向会话内所有成员广播同步通知（play/pause/seek/stop）
    void BroadcastSyncNotify(const std::string& session_id, const std::string& stream_id,
                             const std::string& action, long long position_ms, int operator_uid);

    std::thread _worker_thread;
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    bool _b_stop;   
    std::map<short, FunCallBack> _fun_callbacks;
};
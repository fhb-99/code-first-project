#pragma once

#include "global.h"
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class CSession;

class UserMgr : public Singleton<UserMgr>
{
    friend class Singleton<UserMgr>;
public:
    ~UserMgr();
    std::shared_ptr<CSession> GetSession(int uid);
    void SetUserSession(int uid, std::shared_ptr<CSession> session);
    void RemoveUserSession(int uid);
private:
    UserMgr();
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<CSession>> map;
};
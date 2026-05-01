#include "UserMgr.h"

UserMgr::UserMgr()
{

}

UserMgr::~UserMgr()
{
    map.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto itor = map.find(uid);
    if(itor == map.end())
    {
        return nullptr;
    }
    return itor->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(map.find(uid) != map.end())
    {
        map.erase(uid);
    }
    map[uid] = session;
}   

void UserMgr::RemoveUserSession(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(map.find(uid) == map.end()) 
    {
        return;
    }
    map.erase(uid);
}
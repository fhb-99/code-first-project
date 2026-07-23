#pragma once

#include <functional>

#include "global.h"
#include "HttpConnection.h"
#include "VarifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include "ConfigMgr.h"

class HttpConnection;

typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>//, public std::enable_shared_from_this<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem() {}
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
    void RegisterGet(std::string, HttpHandler handler);

    bool HandlePost(std::string url, std::shared_ptr<HttpConnection>);
    void RegisterPost(std::string url, HttpHandler handler);
private:
    LogicSystem();
    std::map<std::string, HttpHandler> _post_handlers;
    std::map<std::string, HttpHandler> _get_handlers;
};
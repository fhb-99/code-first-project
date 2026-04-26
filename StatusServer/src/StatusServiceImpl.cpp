#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "global.h"

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


std::string generate_unique_string() {
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);

    return unique_string;
}

StatusServiceImpl::StatusServiceImpl()
{
    auto& cfg = ConfigMgr::Inst();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    _servers.push_back(server);

    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    _servers.push_back(server);
}

Status StatusServiceImpl::GetChatServer(ServerContext *context, const GetChatServerRequest *request, GetChatServerRsponse *reply)
{
    std::string prefix("llfc status server has received :  ");
    if(_servers.empty())
    {
        reply->set_error(ErrorCodes::RPCFailed);
        return Status::OK;
    }

    const auto uid = request->uid();
    const auto token = generate_unique_string();
    const auto idx = _server_index.fetch_add(1) % _servers.size();
    auto &server = _servers[idx];
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(token);

    {
        std::lock_guard<std::mutex> guard(_token_mtx);
        _tokens[uid] = token;
    }
    std::cout << "get ChatServer ip: " << server.host << std::endl;
    std::cout << "port is: " << server.port << std::endl;
    return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginRequest* request,
		LoginRsponse* reply) 
{
    const auto uid = request->uid();
    const auto token = request->token();
    std::lock_guard<std::mutex> guard(_token_mtx);
    auto iter = _tokens.find(uid);
    if (iter == _tokens.end()) 
    {
        reply->set_error(ErrorCodes::UidInvalid);
        return Status::OK;
    }
    if (iter->second != token) 
    {
        reply->set_error(ErrorCodes::TokenInvalid);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    std::cout << "uid is: " << uid << "token is: " << token << std::endl;
    return Status::OK;
}
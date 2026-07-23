#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerRequest;
using message::GetChatServerRsponse;
using message::LoginRequest;
using message::LoginRsponse;
using message::StatusService;

struct ChatServer {
    std::string host;
    std::string port;
};

class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, 
        const GetChatServerRequest* request, GetChatServerRsponse* reply) override;

    Status Login(ServerContext* context, const LoginRequest* request, LoginRsponse* reply) override;

    std::vector<ChatServer> _servers;
    std::atomic<std::size_t> _server_index{0};
    // Kept for compatibility/debug; Redis is the source of truth for tokens.
    std::unordered_map<int, std::string> _tokens;
    std::mutex _token_mtx;
    //int _server_index;
};

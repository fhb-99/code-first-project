#pragma once

#include "message.grpc.pb.h"
#include "message.pb.h"
#include "global.h"
#include "CSession.h"
#include <mutex>

using grpc::Server;
using grpc::Status;
using grpc::ServerBuilder;
using grpc::ServerContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

//using message::GetChatServerRsp;
using message::LoginRsponse;
using message::LoginRequest;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


class ChatServiceImpl : public ChatService::Service
{
public:
    ChatServiceImpl();
    Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request, 
                            AddFriendRsp * reply) override;

    Status NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
                            AuthFriendRsp* reply) override;

    Status NotifyTextChatMsg(::grpc::ServerContext* context, const TextChatMsgReq* request, 
                            TextChatMsgRsp* reply) override;

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
private:

};
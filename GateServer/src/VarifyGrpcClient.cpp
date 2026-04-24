#include "VarifyGrpcClient.h"

VarifyGrpcClient::VarifyGrpcClient()
{
    auto& gCfgMgr = ConfigMgr::Inst();
    std::string host = gCfgMgr["VarifyServer"]["Host"];
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    _pool.reset(new RPConPool(5, host, port));
}

GetVarifyRsponse VarifyGrpcClient::GetVarifyCode(std::string email)
{
    GetVarifyRsponse reply;
    GetVarifyRequest request;
    request.set_email(email);

    auto stub = _pool->getConnection();
    if (!stub) {
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }

    ClientContext context;
    Status status = stub->GetVarifyCode(&context, request, &reply);
    _pool->returnConnection(std::move(stub));

    if (!status.ok()) {
        reply.set_error(ErrorCodes::RPCFailed);
    }
    return reply;
}
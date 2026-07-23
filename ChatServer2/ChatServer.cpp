#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "ChatGrpcClient.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"


bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
    auto &cfg = ConfigMgr::Inst();
    auto server_name = cfg["SelfServer"]["Name"];
    auto pool = AsioIOServicePool::GetInstance();
    try {

        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

        
        const std::string rpc_host = cfg["SelfServer"]["Host"];
        const std::string rpc_port = cfg["SelfServer"]["RPCPort"];
        std::string server_address(rpc_host + ":" + rpc_port);
        ChatServiceImpl service;
        grpc::ServerBuilder builder;

        //监听端口和添加服务
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);

        //构建并且启动grpc服务
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        if (!server) {
            std::cerr << "Failed to start gRPC server on " << server_address << std::endl;
            throw std::runtime_error("grpc BuildAndStart failed");
        }
        std::cout << "RPC Server listening on : " << server_address << std::endl;

        //单独起一个线程处理grpc调用
        std::thread grpc_server_thread([&server]() { server->Wait(); });

        boost::asio::io_context  io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool, &server](auto, auto) {
            io_context.stop();
            pool->Stop();
            server->Shutdown();
        });
        auto port_str = cfg["SelfServer"]["Port"];
        unsigned short port = static_cast<unsigned short>(atoi(port_str.c_str()));
        CServer s(io_context, port);
        io_context.run();
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::GetInstance()->Close();
		grpc_server_thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
    }

}

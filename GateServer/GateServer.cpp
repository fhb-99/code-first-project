#include <iostream>
#include <boost/version.hpp> 

#include "global.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "LogicSystem.h"
#include "VarifyGrpcClient.h"
//测试环境
//using namespace std;
// int main() {
//     cout << "Boost 版本" << BOOST_VERSION << endl;
//     return 0;
// }

int main() 
{
    //grpc_init();  // gRPC 必须在主线程先初始化，否则 worker 线程首次调用 InsecureChannelCredentials 会段错误

    auto& gcfgMgr = ConfigMgr::Inst();
    std::string gate_port_str = gcfgMgr["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());

    // 主线程预创建 VarifyGrpcClient，触发 gRPC 初始化，避免 worker 线程首次调用时段错误
    // (void)VarifyGrpcClient::GetInstance();

    try
    {
        unsigned short port = static_cast<unsigned short>(gate_port);
        net::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) 
        {
            if (error) 
            {
                return;
            }
            ioc.stop();
        });
        std::make_shared<CServer>(ioc, port)->start();
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        //grpc_shutdown();
        return EXIT_FAILURE;
    }

    //grpc_shutdown();
    return 0;
}
#include "global.h"
#include "AsioIOServicePool.h"
#include "ConfigMgr.h"
#include "CServer.h"
#include "LogicSystem.h"
#include <thread>
#include <mutex>
#include <csignal>


int main()
{
    auto& cfg = ConfigMgr::Inst();
    auto server_name = cfg["SelfServer"]["Name"];
    auto pool = AsioIOServicePool::GetInstance();

    try
    {
        const std::string host = cfg["SelfServer"]["Host"];
        const std::string port_str = cfg["SelfServer"]["Port"];
        unsigned short listen_port = static_cast<unsigned short>(std::stoi(port_str));

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool](auto, auto) {
            io_context.stop();
            pool->Stop();
        });

        CServer s(io_context, listen_port);
        io_context.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}


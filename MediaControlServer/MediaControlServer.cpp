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
    // 与 conf/config.ini 中 [MediaControlServer] 段一致
    auto server_name = cfg["MediaControlServer"]["Name"];
    (void)server_name;
    auto pool = AsioIOServicePool::GetInstance();

    try
    {
        const std::string port_str = cfg["MediaControlServer"]["Port"];
        if (port_str.empty())
        {
            std::cerr << "Config error: [MediaControlServer] Port is missing or empty." << std::endl;
            return -1;
        }
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


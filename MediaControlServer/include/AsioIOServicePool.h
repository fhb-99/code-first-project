#pragma once

#include "Singleton.h"
#include "global.h"

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;

    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;

    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
    std::vector<IOService> m_ioservices;
    std::vector<WorkPtr> m_works;
    std::vector<std::thread> m_threads;
    std::size_t m_nextIOService;
};
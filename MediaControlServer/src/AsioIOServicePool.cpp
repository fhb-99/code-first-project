#include "AsioIOServicePool.h"

AsioIOServicePool::~AsioIOServicePool()
{
    Stop();
}


AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : m_ioservices(size), 
    m_works(size), 
    m_nextIOService(0)
{
    for(std::size_t i = 0; i < size; i++)
    {
        m_works[i] = std::unique_ptr<Work> (new Work(m_ioservices[i]));
    }
    for(std::size_t i = 0; i < size; i++)
    {
        m_threads.emplace_back([this, i](){
            m_ioservices[i].run();
        });
    }
}


boost::asio::io_context& AsioIOServicePool::GetIOService()
{
    auto& service = m_ioservices[m_nextIOService++];
    if(m_nextIOService == m_ioservices.size())
    {
        m_nextIOService = 0;
    }
    return service;
}


void AsioIOServicePool::Stop()
{
    for(auto& work : m_works)
    {
        work->get_io_context().stop();
        work.reset();
    }
    for(auto& thread : m_threads)
    {
        thread.join();
    }
}


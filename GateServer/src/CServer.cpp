#include "CServer.h"
#include "AsioIOServicePool.h"

CServer::CServer(boost::asio::io_context &ioc, unsigned short &port)
    : _ioc(ioc), 
    _acceptor(ioc,  tcp::endpoint(tcp::v4(), port)), 
    _socket(ioc)
{

}

void CServer::start()
{
    auto self = shared_from_this();
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
    _acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code error)
    {
        try
        {
            if(error)  //如果出错，则放弃当前连接,监听新连接
            {
                self->start();
                return;
            }

            //处理新连接
            // std::make_shared<HttpConnection>(std::move(self->_socket))->start();
            new_con->start();
            //继续监听
            self->start();
        }
        catch (std::exception& exp) 
        {
            std::cout << "exception is " << exp.what() << std::endl;
            self->start();
        }
    });
}

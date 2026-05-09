#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <unordered_map>
#include <string>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/version.hpp>
#include <grpc/grpc.h>
#include <atomic>
#include <queue>
#include <mutex>
#include <memory>
#include <iostream>
#include <condition_variable>
#include <map>
#include <queue>
#include <function>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


#define MAX_LENGTH 1024 * 2  //读缓冲区的大小
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2

// RAII思想
class Defer
{
public:
    Defer(std::function<void()> func) : m_func(func) {}
    ~Defer() { m_func(); }
private:
    std::function<void()> m_func;
};


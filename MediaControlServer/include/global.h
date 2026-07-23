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
#include <functional>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


enum ReqID
{
    ID_MEDIA_LIST_REQ = 1020, //媒体列表请求
    ID_MEDIA_LIST_RSP = 1021, //媒体列表回包
    ID_MEDIA_PLAY_REQ = 1022, //媒体播放请求
    ID_MEDIA_PLAY_RSP = 1023, //媒体播放回包
    ID_MEDIA_STOP_REQ = 1024, //媒体停止请求
    ID_MEDIA_STOP_RSP = 1025, //媒体停止回包
    ID_MEDIA_SYNC_NOTIFY = 1026, //媒体同步通知
    ID_MEDIA_LAYOUT_REQ = 1027, //媒体布局请求
    ID_MEDIA_LAYOUT_NOTIFY = 1028, //媒体布局通知
    ID_MEDIA_HEARTBEAT_REQ = 1029, //媒体心跳请求
    ID_MEDIA_HEARTBEAT_RSP = 1030, //媒体心跳回包
    ID_MEDIA_SESSION_LIST_REQ = 1031, //媒体会话列表请求
    ID_MEDIA_SESSION_LIST_RSP = 1032, //媒体会话列表回包
    ID_MEDIA_CREATE_SESSION_REQ = 1033, //创建媒体会话请求
    ID_MEDIA_CREATE_SESSION_RSP = 1034, //创建媒体会话回包
    ID_MEDIA_JOIN_SESSION_REQ = 1035, //加入媒体会话请求
    ID_MEDIA_JOIN_SESSION_RSP = 1036, //加入媒体会话回包
    ID_MEDIA_PAUSE_REQ = 1037, //媒体暂停请求（与 New_Client 1026+ 编号对齐后追加）
    ID_MEDIA_PAUSE_RSP = 1038, //媒体暂停回包
};


enum ErrorCodes
{
    Success = 0,
    Error_Json = 1,
    Error_Redis = 2,
    Error_Mysql = 3,
    Error_StreamID = 4,
    Error_NotOwner = 5,        // 非会话 owner 无权执行播放控制操作
    Error_SessionNotFound = 6  // 指定的会话不存在
};


#define MAX_LENGTH 1024 * 2  //读缓冲区的大小
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_SENDQUE 1000

// RAII思想
class Defer
{
public:
    Defer(std::function<void()> func) : m_func(func) {}
    ~Defer() { m_func(); }
private:
    std::function<void()> m_func;
};


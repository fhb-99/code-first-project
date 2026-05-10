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
    ID_MEDIA_LEAVE_SESSION_REQ = 1037, //离开媒体会话请求
    ID_MEDIA_LEAVE_SESSION_RSP = 1038, //离开媒体会话回包
    ID_MEDIA_CLOSE_SESSION_REQ = 1039, //关闭媒体会话请求
    ID_MEDIA_CLOSE_SESSION_RSP = 1040, //关闭媒体会话回包
    ID_MEDIA_PAUSE_SESSION_REQ = 1041, //暂停媒体会话请求
    ID_MEDIA_PAUSE_SESSION_RSP = 1042, //暂停媒体会话回包
    ID_MEDIA_RESUME_SESSION_REQ = 1043, //恢复媒体会话请求
    ID_MEDIA_RESUME_SESSION_RSP = 1044, //恢复媒体会话回包
    ID_MEDIA_SEEK_SESSION_REQ = 1045, //跳转媒体会话请求
    ID_MEDIA_SEEK_SESSION_RSP = 1046, //跳转媒体会话回包
    ID_MEDIA_VOLUME_SESSION_REQ = 1047, //设置媒体会话音量请求
    ID_MEDIA_VOLUME_SESSION_RSP = 1048, //设置媒体会话音量回包
    ID_MEDIA_MUTE_SESSION_REQ = 1049, //静音媒体会话请求
    ID_MEDIA_MUTE_SESSION_RSP = 1050, //静音媒体会话回包
    ID_MEDIA_UNMUTE_SESSION_REQ = 1051, //取消静音媒体会话请求
    ID_MEDIA_UNMUTE_SESSION_RSP = 1052, //取消静音媒体会话回包
    ID_MEDIA_SCREEN_CAPTURE_REQ = 1053, //屏幕捕获请求
    ID_MEDIA_SCREEN_CAPTURE_RSP = 1054, //屏幕捕获回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1055, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1056, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1057, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1058, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1059, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1060, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1061, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1062, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1063, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1064, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1065, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1066, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1067, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1068, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1069, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1070, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1071, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1072, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1073, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1074, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1075, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1076, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1077, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1078, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1079, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1080, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1081, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1082, //屏幕捕获通知
    ID_MEDIA_SCREEN_CAPTURE_STOP_REQ = 1083, //屏幕捕获停止请求
    ID_MEDIA_SCREEN_CAPTURE_STOP_RSP = 1084, //屏幕捕获停止回包
    ID_MEDIA_SCREEN_CAPTURE_NOTIFY = 1085, //屏幕捕获通知
};


enum ErrorCodes
{
    Success = 0,
    Error_Json = 1001,
};


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


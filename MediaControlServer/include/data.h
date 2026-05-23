#pragma once

#include <string>


struct MediaListInfo{
    MediaListInfo() : id(0), stream_id(""), name(""), url(""), source_type(0), status(0), owner_id(0) {}    
    int id;
    std::string stream_id;
    std::string name;
    std::string url;
    int source_type;
    int status;
    int owner_id;
};


struct SessionInfo{
    SessionInfo() : session_id(""), session_name(""), owner_id(0), current_stream_id(""), current_pos_ms(0), sync_version(0), state(0) {}
    std::string session_id;
    std::string session_name;
    int owner_id;
    std::string current_stream_id;
    long long current_pos_ms;
    int sync_version;
    int state;
};

// 会话成员信息，用于广播同步通知时查询成员列表
struct SessionMemberInfo{
    SessionMemberInfo() : uid(0), session_id("") {}
    int uid;
    std::string session_id;
};
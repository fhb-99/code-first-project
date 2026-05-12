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
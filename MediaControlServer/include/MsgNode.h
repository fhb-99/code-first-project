#pragma once

#include "global.h"

class MsgNode
{
public:
     MsgNode(short total_len, short cur_len) : m_total_len(total_len), m_cur_len(cur_len)
     {
        data = new char[m_total_len + 1]();
        data[m_total_len] = '\0';
     }

     ~MsgNode()
     {
        delete[] data;
     }

     void clear()
     {
        memset(data, 0, m_total_len);
        m_cur_len = 0;
     }

     short m_total_len;
     short m_cur_len;
     char * data;
};


class RecvNode : public MsgNode
{
public:
    RecvNode(short len, short msg_id);
    ~RecvNode() = default;
private:
    short m_msg_id;
};

class SendNode : public MsgNode
{
public:
    SendNode(short len, short msg_id);
    ~SendNode() = default;
private:
    short m_msg_id;
};
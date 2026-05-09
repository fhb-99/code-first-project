#include "MsgNdoe.h"

RecvNode::RecvNode(short len, short msg_id) : m_msg_id(msg_id)
{

}


SendNode::SendNode(short len, short msg_id) : m_msg_id(msg_id)
{
    //发送时要将本地字节序转为网络字节序 ——这里是大端
}
#include "MsgNode.h"

RecvNode::RecvNode(short len, short msg_id)
    : MsgNode(len, 0),
      m_msg_id(msg_id)
{
}


SendNode::SendNode(const char* msg, short max_len, short msg_id)
    : MsgNode(static_cast<short>(max_len + HEAD_TOTAL_LEN), 0),
      m_msg_id(msg_id)
{
    const short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(data, &msg_id_net, HEAD_ID_LEN);

    const short max_len_net = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(data + HEAD_ID_LEN, &max_len_net, HEAD_DATA_LEN);

    if (msg != nullptr && max_len > 0)
    {
        memcpy(data + HEAD_TOTAL_LEN, msg, static_cast<std::size_t>(max_len));
    }
}
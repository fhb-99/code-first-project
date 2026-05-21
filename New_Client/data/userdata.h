#ifndef USERDATA_H
#define USERDATA_H
#include <QString>
#include <memory>
#include <QJsonArray>
#include <vector>
#include <QJsonObject>
#include <string>
#include <cstdint>

class SearchInfo {
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon);
	int _uid;
	QString _name;
	QString _nick;
	QString _desc;
	int _sex;
    QString _icon;
};

class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString desc,
                   QString icon, QString nick, int sex);
	int _from_uid;
	QString _name;
	QString _desc;
    QString _icon;
    QString _nick;
    int     _sex;
};

struct ApplyInfo {
    ApplyInfo(int uid, QString name, QString desc,
        QString icon, QString nick, int sex, int status)
        :_uid(uid),_name(name),_desc(desc),
        _icon(icon),_nick(nick),_sex(sex),_status(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        :_uid(addinfo->_from_uid),_name(addinfo->_name),
          _desc(addinfo->_desc),_icon(addinfo->_icon),
          _nick(addinfo->_nick),_sex(addinfo->_sex),
          _status(0)
    {}
    void SetIcon(QString head){
        _icon = head;
    }
    int _uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
};

struct AuthInfo {
    AuthInfo(int uid, QString name,
             QString nick, QString icon, int sex):
        _uid(uid), _name(name), _nick(nick), _icon(icon),
        _sex(sex){}
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
};

struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name,
            QString peer_nick, QString peer_icon, int peer_sex)
        :_uid(peer_uid),_name(peer_name),_nick(peer_nick),
          _icon(peer_icon),_sex(peer_sex)
    {}

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
};

struct TextChatData;
struct FriendInfo {
    FriendInfo(int uid, QString name, QString nick, QString icon,
        int sex, QString desc, QString back, QString last_msg=""):_uid(uid),
        _name(name),_nick(nick),_icon(icon),_sex(sex),_desc(desc),
        _back(back),_last_msg(last_msg){}

    FriendInfo(std::shared_ptr<AuthInfo> auth_info):_uid(auth_info->_uid),
    _nick(auth_info->_nick),_icon(auth_info->_icon),_name(auth_info->_name),
      _sex(auth_info->_sex){}

    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp):_uid(auth_rsp->_uid),
    _nick(auth_rsp->_nick),_icon(auth_rsp->_icon),_name(auth_rsp->_name),
      _sex(auth_rsp->_sex){}

    void AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData>> text_vec);

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _desc;
    QString _back;
    QString _last_msg;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct UserInfo {
    UserInfo(int uid, QString name, QString nick, QString icon, int sex, QString last_msg = ""):
        _uid(uid),_name(name),_nick(nick),_icon(icon),_sex(sex),_last_msg(last_msg){}

    UserInfo(std::shared_ptr<AuthInfo> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_last_msg(""){}

    UserInfo(int uid, QString name, QString icon):
    _uid(uid), _name(name), _icon(icon),_nick(_name),
    _sex(0),_last_msg(""){

    }

    UserInfo(std::shared_ptr<AuthRsp> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_last_msg(""){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
        _uid(search_info->_uid),_name(search_info->_name),_nick(search_info->_nick),
    _icon(search_info->_icon),_sex(search_info->_sex),_last_msg(""){

    }

    UserInfo(std::shared_ptr<FriendInfo> friend_info):
        _uid(friend_info->_uid),_name(friend_info->_name),_nick(friend_info->_nick),
        _icon(friend_info->_icon),_sex(friend_info->_sex),_last_msg(""){
            _chat_msgs = friend_info->_chat_msgs;
        }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _last_msg;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct TextChatData{
    TextChatData(QString msg_id, QString msg_content, int fromuid, int touid)
        :_msg_id(msg_id),_msg_content(msg_content),_from_uid(fromuid),_to_uid(touid){

    }
    QString _msg_id;
    QString _msg_content;
    int _from_uid;
    int _to_uid;
};

struct TextChatMsg{
    TextChatMsg(int fromuid, int touid, QJsonArray arrays):
        _from_uid(fromuid),_to_uid(touid){
        for(auto  msg_data : arrays){
            auto msg_obj = msg_data.toObject();
            auto content = msg_obj["content"].toString();
            auto msgid = msg_obj["msgid"].toString();
            auto msg_ptr = std::make_shared<TextChatData>(msgid, content,fromuid, touid);
            _chat_msgs.push_back(msg_ptr);
        }
    }
    int _to_uid;
    int _from_uid;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};
enum class StreamSourceType : std::uint8_t {
    Unknown = 0,
    Rtsp = 1,
    Hls = 2,
    File = 3,
    Http = 4
};

enum class StreamCodecType : std::uint8_t {
    Unknown = 0,
    H264 = 1,
    H265 = 2,
    Aac = 3,
    Opus = 4
};

enum class PlayState : std::uint8_t {
    Idle = 0,
    Playing = 1,
    Paused = 2,
    Buffering = 3,
    Stopped = 4
};

enum class ControlCommandType : std::uint8_t {
    Unknown = 0,
    Play = 1,
    Stop = 2,
    Switch = 3,
    Layout = 4,
    Pause = 5,
    Resume = 6,
    Seek = 7,
    Volume = 8
};

enum class SyncScope : std::uint8_t {
    Local = 0,
    Session = 1
};

// 播放器相关的数据封装
struct StreamInfo {
    StreamInfo() = default;
    StreamInfo(std::string s_id, std::string name_, std::string url_, std::string type_, std::string codec_)
        : streamId(std::move(s_id)), name(std::move(name_)), url(std::move(url_)),
          sourceTypeStr(std::move(type_)), codecStr(std::move(codec_))
    {}

    std::string streamId;
    std::string name;
    std::string url;
    StreamSourceType sourceType = StreamSourceType::Unknown;
    StreamCodecType videoCodec = StreamCodecType::Unknown;
    StreamCodecType audioCodec = StreamCodecType::Unknown;
    std::string sourceTypeStr;   // 兼容字段: rtsp / file / http / hls
    std::string codecStr;        // 兼容字段: h264 / h265
    bool hasAudio = false;
    bool hasVideo = true;
    int ownerUid = 0;
    int status = 0;              // 0 unknown, 1 online, 2 offline, 3 disabled
    std::int64_t createdAtMs = 0;
    std::int64_t updatedAtMs = 0;
};

// 单个会话槽位上的播放状态（不是“会话主表”）
struct StreamSession {
    StreamSession() = default;
    StreamSession(int idx, std::string s_id, std::string url_)
        : slotIndex(idx), streamId(std::move(s_id)), url(std::move(url_))
    {}

    std::string sessionId;
    int slotIndex = -1;
    std::string streamId;
    std::string url;
    PlayState playState = PlayState::Idle;
    bool isSyncControlled = false;
    std::int64_t positionMs = 0;
    int volume = 100;
    int updatedByUid = 0;
    std::int64_t updatedAtMs = 0;
};

struct ControlCommand {
    ControlCommand() = default;
    ControlCommand(std::string comm, std::string s_id, int lay_idx)
        : command(std::move(comm)), streamId(std::move(s_id)), layoutIndex(lay_idx)
    {}

    std::string cmdId;
    std::string sessionId;
    int operatorUid = 0;
    ControlCommandType commandType = ControlCommandType::Unknown;
    std::string command;   // 兼容字段: play / stop / switch / layout ...
    std::string streamId;
    int layoutIndex = -1;
    std::int64_t seekPositionMs = -1;
    int volume = -1;
    SyncScope syncScope = SyncScope::Local;
    bool sync = false;     // 兼容字段，建议逐步迁移到 syncScope
    std::int64_t createdAtMs = 0;
};

#endif

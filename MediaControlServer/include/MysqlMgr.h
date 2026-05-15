#pragma once

#include "global.h"
#include "Singleton.h"
#include "data.h"
#include <mysql/mysql.h>
#include <mysql/mysql_time.h>
#include <mysql/my_command.h>
#include <mysql/my_compress.h>
#include <mysql/my_list.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>  // 定义 sql::PreparedStatement
#include <cppconn/resultset.h>            // 定义 sql::ResultSet
#include <cppconn/exception.h>
#include <mysql_driver.h>

class SqlConnection
{
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) : _con(con), _last_oper_time(lasttime) {}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MysqlPool {
public:
    MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false){
        try {
            for (int i = 0; i < poolSize_; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                auto*  con = driver->connect(url_, user_, pass_);
                con->setSchema(schema_);
                // 获取当前时间戳
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				// 将时间戳转换为秒
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));
            }
        }
        catch (sql::SQLException& e) {
            std::cerr << "MysqlPool init SQLException: " << e.what()
                      << " (code=" << e.getErrorCode() << ", SQLState=" << e.getSQLState() << ")"
                      << std::endl;
        }
    }

    std::unique_ptr<SqlConnection> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { 
            if (b_stop_) {
                return true;
            }        
            return !pool_.empty(); });
        if (b_stop_) {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con));
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

    ~MysqlPool() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
};


class MysqlMgr : public Singleton<MysqlMgr>
{
public:
    MysqlMgr();
    ~MysqlMgr();
    // int RegisterUser(const std::string& name, 
    //             const std::string& email, const std::string& pwd);

    // bool CheckEmail(const std::string& name, const std::string & email);
	// bool UpdatePwd(const std::string& name, const std::string& newpwd);
    // bool CheckPwd(const std::string& email, 
    //             const std::string& pwd, UserInfo& userInfo);

    // std::shared_ptr<UserInfo> GetUser(int uid);
    // std::shared_ptr<UserInfo> GetUser(std::string name);

    // bool AddFriendApply(const int& from, const int& to);
    // bool AuthFriendApply(const int& from, const int& to);
    // bool AddFriend(const int& from, const int& to, std::string back_name);

    // bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit);
    // bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list);

    //获取媒体列表
    bool GetMediaList(int uid, std::vector<std::shared_ptr<MediaListInfo>>& media_list);
    //获取媒体播放流的stream_id
    bool GetStreamInfo(int uid, std::string& stream_id);

    // 更新 media_client_playing.state（与 db/mysql_media_migration.sql 一致）
    bool UpdateMediaPlayStatus(int uid, const std::string& session_id, const std::string& stream_id, int state);
    // 写入media_client_playing表
    bool InsertMediaClientPlaying(int uid, const std::string& session_id, const std::string& stream_id, int state);
    //当客户端播放视频时，会创建一个session_id,插入到media_session_stream表当中
    bool InsertMediaSessionStream(int uid, const std::string& session_id, const std::string& stream_id, int state);
    //同时，当有客户端进入房间时，并且播放的是同一个流时，要维护在线人数信息
    bool UpdateMediaSessionOnlineCount(int uid, const std::string& session_id, const std::string& stream_id, bool is_add);
private:
    std::unique_ptr<MysqlPool> pool_;
};
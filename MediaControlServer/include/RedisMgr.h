#pragma once

#include "global.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <sw/redis++/redis++.h>


class RedisConPool
{
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : b_stop_(false),
          poolSize_(poolSize),
          host_(host ? host : ""),
          port_(port),
          pwd_(pwd ? pwd : ""),
          created_(0)
    {
        for (size_t i = 0; i < poolSize_; ++i) 
        {
            try 
            {
                // 构建连接字符串
                // redis-plus-plus 要求连接串必须要带协议scheme
                std::string conn_str;
                if (host_.find("://") == std::string::npos) {
                    conn_str = "tcp://" + host_ + ":" + std::to_string(port_);
                } else {
                    conn_str = host_;
                    const bool is_unix = (conn_str.rfind("unix://", 0) == 0);
                    if (!is_unix) {
                        const auto scheme_pos = conn_str.find("://");
                        const auto host_pos = (scheme_pos == std::string::npos) ? 0 : (scheme_pos + 3);
                        if (conn_str.find(':', host_pos) == std::string::npos) {
                            conn_str += ":" + std::to_string(port_);
                        }
                    }
                }
                
                if (!pwd_.empty()) {
                    //conn_str += "?password=" + std::string(pwd);
                }
                // 创建 redis-plus-plus 客户端（替代 redisContext）
                auto conn = std::make_shared<sw::redis::Redis>(conn_str);
                
                // 认证（redis-plus-plus 已自动处理，这里显式认证兼容你的逻辑）
                if (!pwd_.empty()) {
                    conn->auth(pwd_);
                    std::cout << "Redis 认证成功" << std::endl;
                }

                // 验证连接（替代 redisConnect 后的 err 检查）
                std::string pong = conn->ping();
                if (pong != "PONG") {
                    throw std::runtime_error("ping failed");
                }

                connections_.push(conn);
                ++created_;
            } 
            catch (const std::exception& e) 
            {
                std::cerr << "创建 Redis 连接失败: " << e.what() << std::endl;
                continue;
            }
        }
    }

    ~RedisConPool()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) 
        {
            connections_.pop();
        }
    }

    // 获取连接（阻塞等待，停止时返回null）→ 替代 redisContext* getConnection()
    std::shared_ptr<sw::redis::Redis> getConnection() 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 仿你的等待逻辑：停止/有空闲连接时唤醒
        cond_.wait(lock, [this] {
            if (b_stop_) return true;
            return !connections_.empty();
        });

        if (b_stop_) return nullptr;

        auto conn = connections_.front();
        connections_.pop();
        return conn;
    }

    // 归还连接 → 替代 void returnConnection(redisContext*)
    void returnConnection(std::shared_ptr<sw::redis::Redis> conn) {
        if (!conn) return;

        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) 
        {
            return; // 停止后不归还
        }
        connections_.push(conn);
        cond_.notify_one(); // 唤醒等待的线程
    }

    // 关闭连接池 → 完全仿你的 Close 逻辑
    void Close() 
    {
        b_stop_ = true;
        cond_.notify_all(); // 唤醒所有等待线程
    }

    bool Healthy() const
    {
        return !b_stop_ && created_.load() > 0;
    }
private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    int port_;
    std::string pwd_;
    std::queue<std::shared_ptr<sw::redis::Redis>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<size_t> created_;
};


class RedisMgr : public Singleton<RedisMgr>, 
                public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();

    // 连接 Redis（支持无密码/有密码）
    bool Connect(const std::string& host, int port, const std::string& password = "");
    
    // 基础 KV 操作
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value, int expire_seconds = 0); // 可选过期时间
    
    // 认证（单独调用，也可在 Connect 里自动调用）
    bool Auth(const std::string& password);
    
    // List 操作
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    
    // Hash 操作（重载，支持 string/char*）
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    
    // 键操作
    bool Del(const std::string& key);
    bool HDel(const std::string& key, const std::string& field);
    bool ExistsKey(const std::string& key);
    
    // 关闭连接
    void Close();

    // 检查是否已连接
    bool IsConnected() const { return _con_pool && _con_pool->Healthy(); }

private:
    RedisMgr();

    // 连接状态标记
    bool _is_connected = false;

    std::unique_ptr<RedisConPool> _con_pool;
};
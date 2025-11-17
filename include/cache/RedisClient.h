#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <optional>

namespace NeonGlyph {
namespace Cache {

class RedisConnection {
public:
    RedisConnection();
    ~RedisConnection();
    bool connect(const std::string& host, int port);
    bool send(const std::string& data);
    std::optional<std::string> read();
    std::optional<std::string> command(const std::string& cmd);
private:
    void* m_sock;
};

class RedisClientPool {
public:
    RedisClientPool();
    ~RedisClientPool();
    bool init(const std::string& endpoint, int poolSize = 4);
    std::optional<std::string> get(const std::string& key);
    bool set(const std::string& key, const std::string& value);
    std::vector<std::optional<std::string>> mget(const std::vector<std::string>& keys);
    bool mset(const std::vector<std::pair<std::string, std::string>>& kvs);
    std::vector<std::optional<std::string>> pipeline(const std::vector<std::string>& cmds);
    bool subscribe(const std::string& channel, const std::function<void(const std::string&)>& cb);
private:
    std::vector<RedisConnection*> m_pool;
    std::mutex m_mutex;
    std::atomic<bool> m_ready{false};
    std::thread m_subThread;
};

} // namespace Cache
} // namespace NeonGlyph
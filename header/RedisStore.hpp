#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class RedisStore {
public:
  void set(const std::string &key, const std::string &value);
  std::optional<std::string> get(const std::string &key) const;

  static RedisStore &instance() {
    static RedisStore instance;
    return instance;
  }

private:
  std::unordered_map<std::string, std::string> mStore;
  mutable std::mutex mMutex;
};

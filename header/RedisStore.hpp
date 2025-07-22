#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

class RedisStoreValue {
public:
  enum Type { STRING, STREAM };

  virtual ~RedisStoreValue() = default;

  RedisStoreValue(
      Type t,
      std::optional<std::chrono::steady_clock::time_point> exp = std::nullopt)
      : m_type(t), mExpiry(exp) {}

  bool hasExpired() const {
    if (!mExpiry) {
      return false;
    }
    return std::chrono::steady_clock::now() >= mExpiry.value();
  }

  virtual std::unique_ptr<RedisStoreValue> clone() const = 0;

  virtual std::string serialize() const = 0;

  Type getType() const { return m_type; }

  std::string getTypeStr() const {
    static std::unordered_map<Type, std::string> type_str = {
        {STRING, "string"}, {STREAM, "stream"}};
    auto it = type_str.find(m_type);
    if (it == type_str.end()) {
      throw std::runtime_error("Unknown type of redis store value");
    }
    return it->second;
  }

private:
  Type m_type;
  std::optional<std::chrono::steady_clock::time_point> mExpiry;
};

class StringValue : public RedisStoreValue {
public:
  StringValue(const std::string &val,
              std::optional<std::chrono::steady_clock::time_point> exp)
      : RedisStoreValue(STRING, exp), mValue(val) {}

  virtual std::string serialize() const override { return mValue; }

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<StringValue>(*this);
  };

private:
  std::string mValue;
};

class StreamValue : public RedisStoreValue {
public:
  using StreamEntry = std::unordered_map<std::string, std::string>;
  using StreamEntryId = std::string;

  virtual std::string serialize() const override {
    // TODO: Implement it
    return "";
  }

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<StreamValue>(*this);
  };

  StreamValue() : RedisStoreValue(STREAM) {}

  void addEntry(const StreamEntryId &stream_id, StreamEntry stream_entry) {
    mStreams[stream_id].merge(stream_entry);
  }

  StreamEntryId getTopEntry() const;

private:
  std::map<StreamEntryId, StreamEntry> mStreams;
};

class RedisStore {
public:
  void setString(const std::string &key, const std::string &value,
                 std::optional<std::chrono::milliseconds> exp = std::nullopt);

  void addStreamEntry(const std::string &key,
                      const StreamValue::StreamEntryId &entry_id,
                      StreamValue::StreamEntry entry);

  std::optional<std::unique_ptr<RedisStoreValue>>
  get(const std::string &key) const;

  bool keyExists(const std::string &key) const;

  static RedisStore &instance() {
    static RedisStore instance;
    return instance;
  }

private:
  RedisStore() = default;
  std::unordered_map<std::string, std::unique_ptr<RedisStoreValue>> mStore;
  mutable std::mutex mMutex;
};

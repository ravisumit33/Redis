#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

  std::string getValue() const { return mValue; }

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<StringValue>(*this);
  };

private:
  std::string mValue;
};

class StreamValue : public RedisStoreValue {
public:
  using StreamEntry = std::unordered_map<std::string, std::string>;
  using StreamEntryId = std::array<uint64_t, 2>;
  using StreamIterator = std::map<StreamEntryId, StreamEntry>::const_iterator;

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<StreamValue>(*this);
  };

  StreamIterator begin() const { return mStreams.begin(); }

  StreamIterator end() const { return mStreams.end(); }

  bool empty() const { return mStreams.empty(); }

  StreamValue() : RedisStoreValue(STREAM) {}

  void addEntry(const StreamEntryId &stream_id, StreamEntry stream_entry) {
    mStreams[stream_id].merge(stream_entry);
  }

  StreamEntryId getTopEntry() const;

  StreamIterator lowerBound(const StreamEntryId &entry_id) const {
    return mStreams.lower_bound(entry_id);
  }

  StreamIterator upperBound(const StreamEntryId &entry_id) const {
    return mStreams.upper_bound(entry_id);
  }

private:
  std::map<StreamEntryId, StreamEntry> mStreams;
};

class RedisStore {
public:
  void setString(const std::string &key, const std::string &value,
                 std::optional<std::chrono::milliseconds> exp = std::nullopt);

  StreamValue::StreamEntryId addStreamEntry(const std::string &key,
                                            const std::string &entry_id,
                                            StreamValue::StreamEntry entry);

  std::optional<std::unique_ptr<RedisStoreValue>>
  get(const std::string &key) const;

  bool keyExists(const std::string &key) const;

  std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
  getStreamEntriesInRange(const std::string &store_key,
                          const std::string &entry_id_start,
                          const std::string &entry_id_end) const;

  std::pair<bool,
            std::unordered_map<
                std::string, std::vector<std::pair<StreamValue::StreamEntryId,
                                                   StreamValue::StreamEntry>>>>
  getStreamEntriesAfterAny(
      const std::vector<std::string> &store_keys,
      const std::vector<std::string> &entry_ids_start,
      std::optional<uint64_t> timeout_ms = std::nullopt) const;

  static RedisStore &instance() {
    static RedisStore instance;
    return instance;
  }

private:
  RedisStore() = default;
  std::unordered_map<std::string, std::unique_ptr<RedisStoreValue>> mStore;
  mutable std::shared_mutex mMutex;
  mutable std::condition_variable_any m_cv;
};

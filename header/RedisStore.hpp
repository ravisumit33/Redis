#pragma once

#include "utils/FifoBlockingQueue.hpp"
#include <chrono>
#include <cstdint>
#include <deque>
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
  enum Type { STRING, STREAM, LIST };

  virtual ~RedisStoreValue() = default;

  RedisStoreValue(
      Type t,
      std::optional<std::chrono::system_clock::time_point> exp = std::nullopt)
      : m_type(t), mExpiry(exp) {}

  bool hasExpired() const {
    if (!mExpiry) {
      return false;
    }
    return std::chrono::system_clock::now() >= mExpiry.value();
  }

  virtual std::unique_ptr<RedisStoreValue> clone() const = 0;

  Type getType() const { return m_type; }

  std::string getTypeStr() const {
    static std::unordered_map<Type, std::string> type_str = {
        {STRING, "string"}, {STREAM, "stream"}};
    auto it = type_str.find(m_type);
    if (it == type_str.end()) {
      throw std::logic_error("Unknown type of redis store value");
    }
    return it->second;
  }

private:
  Type m_type;
  std::optional<std::chrono::system_clock::time_point> mExpiry;
};

class StringValue : public RedisStoreValue {
public:
  StringValue(const std::string &val,
              std::optional<std::chrono::system_clock::time_point> exp)
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

class ListValue : public RedisStoreValue {
public:
  ListValue(const std::vector<std::string> &elements)
      : RedisStoreValue(LIST), m_elements(elements.begin(), elements.end()) {}

  ListValue &insertAtBegin(const std::vector<std::string> &v) {
    m_elements.insert(m_elements.begin(), v.begin(), v.end());
    return *this;
  }

  ListValue &insertAtEnd(const std::vector<std::string> &v) {
    m_elements.insert(m_elements.end(), v.begin(), v.end());
    return *this;
  }

  bool empty() const { return m_elements.empty(); }

  std::size_t size() const { return m_elements.size(); }

  std::string pop_front() {
    std::string front_el = m_elements.front();
    m_elements.pop_front();
    return front_el;
  }

  std::vector<std::string> getElementsInRange(int start_idx, int end_idx);

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<ListValue>(*this);
  };

private:
  std::deque<std::string> m_elements;
};

class RedisStore {
public:
  void setString(
      const std::string &key, const std::string &value,
      std::optional<std::chrono::system_clock::time_point> exp = std::nullopt);

  std::size_t addListElementsAtEnd(const std::string &key,
                                   const std::vector<std::string> &elements);

  std::size_t addListElementsAtBegin(const std::string &key,
                                     const std::vector<std::string> &elements);

  std::pair<bool, std::vector<std::string>>
  removeListElementsAtBegin(const std::string &key, unsigned int count,
                            std::optional<double> timeout_s = std::nullopt);

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

  std::vector<std::string> getKeys() const;

private:
  RedisStore() = default;
  std::unordered_map<std::string, std::unique_ptr<RedisStoreValue>> m_store;
  mutable std::shared_mutex m_mutex;
  mutable std::unordered_map<std::string, FifoBlockingQueue> m_blocking_queues;
  mutable std::mutex m_blocking_queues_mutex;

  void notifyBlockingClients(const std::string &key) const;

  class StoreReadGuard {
  public:
    explicit StoreReadGuard(const RedisStore &store)
        : m_store(store.m_store), m_lock(store.m_mutex) {}

    const auto &operator*() const { return m_store; }
    const auto *operator->() const { return &m_store; }

  private:
    const std::unordered_map<std::string, std::unique_ptr<RedisStoreValue>>
        &m_store;
    std::shared_lock<std::shared_mutex> m_lock;
  };

  class StoreWriteGuard {
  public:
    explicit StoreWriteGuard(const RedisStore &store)
        : m_store(const_cast<std::unordered_map<
                      std::string, std::unique_ptr<RedisStoreValue>> &>(
              store.m_store)),
          m_lock(store.m_mutex) {}

    auto &operator*() { return m_store; }
    auto *operator->() { return &m_store; }

  private:
    std::unordered_map<std::string, std::unique_ptr<RedisStoreValue>> &m_store;
    std::unique_lock<std::shared_mutex> m_lock;
  };

  class BlockingQueuesGuard {
  public:
    explicit BlockingQueuesGuard(const RedisStore &store)
        : m_queues(
              const_cast<std::unordered_map<std::string, FifoBlockingQueue> &>(
                  store.m_blocking_queues)),
          m_lock(store.m_blocking_queues_mutex) {}

    auto &operator*() { return m_queues; }
    auto *operator->() { return &m_queues; }

    FifoBlockingQueue &getQueue(const std::string &key) {
      return m_queues[key];
    }

  private:
    std::unordered_map<std::string, FifoBlockingQueue> &m_queues;
    std::unique_lock<std::mutex> m_lock;
  };

public:
  StoreReadGuard readStore() const { return StoreReadGuard(*this); }
  StoreWriteGuard writeStore() const { return StoreWriteGuard(*this); }
  BlockingQueuesGuard blockingQueues() const {
    return BlockingQueuesGuard(*this);
  }
};

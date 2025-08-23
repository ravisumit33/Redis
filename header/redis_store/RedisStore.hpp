#pragma once

#include "redis_store/values/RedisStoreValue.hpp"
#include "redis_store/values/StreamValue.hpp"
#include "utils/FifoBlockingQueue.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

  std::size_t addMemberToSet(const std::string &key, double score,
                             const std::string &member);

  unsigned int getSetMemberRank(const std::string &key,
                                const std::string &member);

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
  mutable std::shared_mutex m_blocking_queues_mutex;

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

  class BlockingQueueGuard {
  public:
    explicit BlockingQueueGuard(const std::string &key,
                                const RedisStore &store) {
      m_read_lock =
          std::shared_lock<std::shared_mutex>(store.m_blocking_queues_mutex);
      auto it = store.m_blocking_queues.find(key);
      if (it != store.m_blocking_queues.end()) {
        m_queue_ptr = &(it->second);
        return;
      }
      m_read_lock.unlock();

      m_write_lock =
          std::unique_lock<std::shared_mutex>(store.m_blocking_queues_mutex);
      it = store.m_blocking_queues.find(key);
      if (it == store.m_blocking_queues.end()) {
        auto [it, _] = store.m_blocking_queues.try_emplace(key);
        m_queue_ptr = &(it->second);
      }
    }

    auto &operator*() { return *m_queue_ptr; }
    auto *operator->() { return m_queue_ptr; }

  private:
    FifoBlockingQueue *m_queue_ptr;
    std::shared_lock<std::shared_mutex> m_read_lock;
    std::unique_lock<std::shared_mutex> m_write_lock;
  };

public:
  StoreReadGuard readStore() const { return StoreReadGuard(*this); }

  StoreWriteGuard writeStore() const { return StoreWriteGuard(*this); }

  BlockingQueueGuard getBlockingQueue(const std::string &key) const {
    return BlockingQueueGuard(key, *this);
  }
};

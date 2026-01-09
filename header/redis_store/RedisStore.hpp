#pragma once

#include "redis_store/RedisValue.hpp"
#include "utils/FifoBlockingQueue.hpp"
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
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

  std::optional<RedisValue> get(const std::string &key) const;

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
                                const std::string &member) const;

  std::size_t removeMemberFromSet(const std::string &key,
                                  const std::string &member);

  std::vector<std::string> getKeys() const;

  RedisStore() = default;
  ~RedisStore() = default;

  RedisStore(const RedisStore &) = delete;
  RedisStore &operator=(const RedisStore &) = delete;
  RedisStore(RedisStore &&) = delete;
  RedisStore &operator=(RedisStore &&) = delete;

private:
  using StoreType = std::unordered_map<std::string, RedisValue>;
  StoreType m_store;
  mutable std::shared_mutex m_mutex;

  mutable std::unordered_map<std::string, FifoBlockingQueue> m_blocking_queues;
  mutable std::shared_mutex m_blocking_queues_mutex;

  void notifyBlockingClients(const std::string &key) const;

  bool waitForListElements(const std::string &key, unsigned int &count,
                           double timeout_s,
                           const std::function<void()> &pop_elements) const;

  static StreamValue::StreamEntryId
  computeEntryIdForExistingStream(StreamValue &stream,
                                  std::optional<uint64_t> timestamp,
                                  std::optional<uint64_t> sequence);

  using StreamEntriesMap =
      std::unordered_map<std::string,
                         std::vector<std::pair<StreamValue::StreamEntryId,
                                               StreamValue::StreamEntry>>>;

  void
  fillStreamEntries(const std::vector<std::string> &store_keys,
                    const std::vector<std::array<uint64_t, 2>> &base_start_ids,
                    StreamEntriesMap &result) const;

  bool waitForStreamEntries(const std::vector<std::string> &store_keys,
                            uint64_t timeout_ms,
                            const std::function<void()> &fill_entries,
                            const std::function<bool()> &has_entries) const;

  class StoreReadGuard {
  public:
    explicit StoreReadGuard(const RedisStore &store)
        : m_store_ptr(&store.m_store), m_lock(store.m_mutex) {}

    const auto &operator*() const { return *m_store_ptr; }
    const auto *operator->() const { return m_store_ptr; }

  private:
    const StoreType *m_store_ptr;
    std::shared_lock<std::shared_mutex> m_lock;
  };

  class StoreWriteGuard {
  public:
    explicit StoreWriteGuard(RedisStore &store)
        : m_store_ptr(&store.m_store), m_lock(store.m_mutex) {}

    auto &operator*() { return *m_store_ptr; }
    auto *operator->() { return m_store_ptr; }

  private:
    StoreType *m_store_ptr;
    std::unique_lock<std::shared_mutex> m_lock;
  };

  class BlockingQueueGuard {
  public:
    explicit BlockingQueueGuard(const std::string &key,
                                const RedisStore &store) {
      {
        auto m_read_lock =
            std::shared_lock<std::shared_mutex>(store.m_blocking_queues_mutex);
        auto iter = store.m_blocking_queues.find(key);
        if (iter != store.m_blocking_queues.end()) {
          m_queue_ptr = &(iter->second);
          return;
        }
      }

      {
        auto m_write_lock =
            std::unique_lock<std::shared_mutex>(store.m_blocking_queues_mutex);
        auto [inserted_iter, inserted] =
            store.m_blocking_queues.try_emplace(key);
        m_queue_ptr = &(inserted_iter->second);
      }

      assert(m_queue_ptr != nullptr);
    }

    auto &operator*() { return *m_queue_ptr; }
    auto *operator->() { return m_queue_ptr; }

  private:
    FifoBlockingQueue *m_queue_ptr = nullptr;
  };

public:
  StoreReadGuard readStore() const { return StoreReadGuard(*this); }

  StoreWriteGuard writeStore() { return StoreWriteGuard(*this); }

  BlockingQueueGuard getBlockingQueue(const std::string &key) const {
    return BlockingQueueGuard(key, *this);
  }
};

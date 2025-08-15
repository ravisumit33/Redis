#include "RedisStore.hpp"
#include "utils/genericUtils.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

StreamValue::StreamEntryId StreamValue::getTopEntry() const {
  if (mStreams.empty()) {
    throw std::logic_error("Stream should not be empty");
  }
  auto it = mStreams.rbegin();
  return it->first;
}

std::vector<std::string> ListValue::getElementsInRange(int start_idx,
                                                       int end_idx) {
  if (empty()) {
    return {};
  }
  std::size_t num_elements = m_elements.size();
  if (start_idx < 0) {
    start_idx += num_elements;
  }
  if (end_idx < 0) {
    end_idx += num_elements;
  }
  start_idx = std::max(start_idx, 0);
  end_idx = std::min<int>(end_idx, num_elements - 1);
  if (start_idx > end_idx) {
    return {};
  }

  std::vector<std::string> elements(m_elements.begin() + start_idx,
                                    m_elements.begin() + end_idx + 1);
  return elements;
}

void RedisStore::setString(
    const std::string &key, const std::string &value,
    std::optional<std::chrono::system_clock::time_point> exp) {
  auto store = writeStore();
  auto val = std::make_unique<StringValue>(value, exp);
  store->insert_or_assign(key, std::move(val));
  notifyBlockingClients(key);
}

std::size_t
RedisStore::addListElementsAtEnd(const std::string &key,
                                 const std::vector<std::string> &elements) {
  auto store = writeStore();
  auto it = store->find(key);

  std::size_t ret = elements.size();
  if (it == store->end()) {
    auto val = std::make_unique<ListValue>(elements);
    store->insert_or_assign(key, std::move(val));
  } else {
    auto &list = static_cast<ListValue &>(*it->second);
    ret += list.size();
    list.insertAtEnd(elements);
  }
  notifyBlockingClients(key);
  return ret;
}

std::size_t
RedisStore::addListElementsAtBegin(const std::string &key,
                                   const std::vector<std::string> &elements) {
  auto store = writeStore();
  auto it = store->find(key);

  std::size_t ret = elements.size();
  if (it == store->end()) {
    auto val = std::make_unique<ListValue>(elements);
    store->insert_or_assign(key, std::move(val));
  } else {
    auto &list = static_cast<ListValue &>(*it->second);
    ret += list.size();
    list.insertAtBegin(elements);
  }
  notifyBlockingClients(key);
  return ret;
}

std::pair<bool, std::vector<std::string>>
RedisStore::removeListElementsAtBegin(const std::string &key,
                                      unsigned int count,
                                      std::optional<double> timeout_s) {

  if (!timeout_s) {
    std::cerr << "No timeout given" << std::endl;
  } else {
  }
  std::vector<std::string> popped_elements;
  auto pop_elements = [&]() {
    auto store = readStore();
    auto it = store->find(key);
    if (it == store->end()) {
      return;
    }
    auto &list = static_cast<ListValue &>(*it->second);
    while (!list.empty() && count--) {
      popped_elements.push_back(list.pop_front());
    }
  };

  pop_elements();

  bool timed_out = false;
  if (count && timeout_s) {
    auto wait_token = blockingQueues().getQueue(key).create_wait_token();

    if (*timeout_s == 0) {
      while (count) {
        wait_token->wait();
        pop_elements();
      }
    } else {
      auto timeout_duration =
          std::chrono::milliseconds(static_cast<uint64_t>(*timeout_s * 1000));
      auto start_time = std::chrono::steady_clock::now();

      while (count) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto remaining =
            timeout_duration -
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

        if (remaining <= std::chrono::milliseconds(0)) {
          std::cerr << "Deadline of " << *timeout_s << " s reached"
                    << std::endl;
          timed_out = true;
          break;
        }

        if (!wait_token->wait_for(remaining)) {
          std::cerr << "Deadline of " << *timeout_s << " s reached"
                    << std::endl;
          timed_out = true;
          break;
        }

        pop_elements();
      }
    }
  }
  return {timed_out, popped_elements};
}

StreamValue::StreamEntryId
RedisStore::addStreamEntry(const std::string &key, const std::string &entry_id,
                           StreamValue::StreamEntry entry) {

  auto nowMs = []() -> uint64_t {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  };

  auto generateSequenceNumber = [](uint64_t timestamp) -> uint64_t {
    return timestamp == 0 ? 1 : 0;
  };

  StreamValue::StreamEntryId saved_entry_id;

  if (entry_id == "*") {
    auto ts = nowMs();
    uint64_t seq = generateSequenceNumber(ts);
    saved_entry_id = {ts, seq};

    auto store = writeStore();
    auto [stream_ptr, inserted] =
        store->try_emplace(key, std::make_unique<StreamValue>());
    static_cast<StreamValue &>(*stream_ptr->second)
        .addEntry(saved_entry_id, std::move(entry));
    notifyBlockingClients(key);
    return saved_entry_id;
  }

  auto [timestamp, sequence] = parsePartialStreamEntryId(entry_id);

  if (timestamp == 0 && sequence == 0) {
    throw std::runtime_error(
        "The ID specified in XADD must be greater than 0-0");
  }

  auto store = writeStore();
  auto it = store->find(key);

  if (it == store->end()) {
    auto stream_ptr = std::make_unique<StreamValue>();
    auto &stream = *stream_ptr;
    uint64_t ts = timestamp.value_or(nowMs());
    uint64_t seq = sequence.value_or(generateSequenceNumber(ts));
    saved_entry_id = {ts, seq};
    stream.addEntry(saved_entry_id, std::move(entry));
    (*store)[key] = std::move(stream_ptr);
  } else {
    auto &stream = static_cast<StreamValue &>(*it->second);
    if (timestamp) {
      uint64_t now = nowMs();
      if (*timestamp > now) {
        throw std::runtime_error("The timestamp in ID is from the future");
      }

      auto [top_ts, top_seq] = stream.getTopEntry();
      if (*timestamp < top_ts ||
          (*timestamp == top_ts && sequence && *sequence <= top_seq)) {
        throw std::runtime_error("The ID specified in XADD is equal or smaller "
                                 "than the target stream top item");
      }

      if (*timestamp == top_ts && !sequence) {
        saved_entry_id = {top_ts, top_seq + 1};
      } else {
        saved_entry_id = {
            *timestamp, sequence.value_or(generateSequenceNumber(*timestamp))};
      }

    } else {
      if (sequence) {
        throw std::runtime_error(
            "Unsupported format: sequence without timestamp");
      }
      uint64_t ts = nowMs();
      auto [top_ts, top_seq] = stream.getTopEntry();
      if (ts < top_ts) {
        throw std::logic_error(
            "Generated timestamp is smaller than top entry's timestamp");
      }
      uint64_t seq = (ts == top_ts ? top_seq + 1 : generateSequenceNumber(ts));
      saved_entry_id = {ts, seq};
    }

    stream.addEntry(saved_entry_id, std::move(entry));
  }

  notifyBlockingClients(key);
  return saved_entry_id;
}

std::optional<std::unique_ptr<RedisStoreValue>>
RedisStore::get(const std::string &key) const {
  auto store = readStore();
  auto it = store->find(key);
  if (it == store->end()) {
    return std::nullopt;
  }

  auto &val = it->second;
  if (val->hasExpired()) {
    return std::nullopt;
  }

  return val->clone();
}

bool RedisStore::keyExists(const std::string &key) const {
  auto store = readStore();
  return store->contains(key);
}

std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
RedisStore::getStreamEntriesInRange(const std::string &store_key,
                                    const std::string &entry_id_start,
                                    const std::string &entry_id_end) const {
  auto store = readStore();
  auto it = store->find(store_key);

  if (it == store->end() || it->second->getType() != RedisStoreValue::STREAM) {
    throw std::runtime_error("Invalid key provided for stream");
  }

  auto stream_val = static_cast<StreamValue &>(*(it->second));
  StreamValue::StreamIterator it1, it2;
  if (entry_id_start == "-") {
    it1 = stream_val.begin();
  } else {
    it1 = stream_val.lowerBound(parseStreamEntryId(entry_id_start));
  }
  if (entry_id_end == "+") {
    it2 = stream_val.end();
  } else {
    it2 = stream_val.upperBound(parseStreamEntryId(entry_id_end));
  }

  std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
      ret;
  while (it1 != it2) {
    ret.push_back({it1->first, it1->second});
    ++it1;
  }
  return ret;
}

std::pair<bool,
          std::unordered_map<std::string,
                             std::vector<std::pair<StreamValue::StreamEntryId,
                                                   StreamValue::StreamEntry>>>>
RedisStore::getStreamEntriesAfterAny(
    const std::vector<std::string> &store_keys,
    const std::vector<std::string> &entry_ids_start,
    std::optional<uint64_t> timeout_ms) const {

  if (store_keys.size() != entry_ids_start.size()) {
    throw std::logic_error("Size of store_keys and corresponding start entry "
                           "ids must be the same");
  }

  std::size_t nkeys = store_keys.size();

  std::optional<std::chrono::system_clock::time_point> deadline;
  if (timeout_ms) {
    deadline = std::chrono::system_clock::now() +
               std::chrono::milliseconds(*timeout_ms);
  }

  std::unordered_map<std::string,
                     std::vector<std::pair<StreamValue::StreamEntryId,
                                           StreamValue::StreamEntry>>>
      ret;

  std::vector<std::array<uint64_t, 2>> base_start_ids;

  auto fillEntries = [&]() {
    auto store = readStore();
    for (std::size_t idx = 0; idx < nkeys; ++idx) {
      auto store_key = store_keys.at(idx);
      auto it = store->find(store_key);
      if (it == store->end()) {
        continue;
      }
      if (it->second->getType() != RedisStoreValue::STREAM) {
        throw std::runtime_error("Invalid key: " + store_key +
                                 " provided for stream");
      }
      auto &stream_val = static_cast<StreamValue &>(*(it->second));
      std::array<uint64_t, 2> start_id = base_start_ids.at(idx);

      std::vector<
          std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
          entries;
      StreamValue::StreamIterator it1 = stream_val.upperBound(start_id);
      while (it1 != stream_val.end()) {
        entries.push_back({it1->first, it1->second});
        ++it1;
      }

      if (!entries.empty()) {
        ret[store_key] = entries;
      }
    }
  };

  {
    auto store = readStore();
    for (std::size_t idx = 0; idx < nkeys; ++idx) {
      auto entry_id_start = entry_ids_start.at(idx);
      if (entry_id_start != "$") {
        base_start_ids.push_back(parseStreamEntryId(entry_id_start));
      } else {
        auto store_key = store_keys.at(idx);
        auto it = store->find(store_key);
        if (it == store->end()) {
          base_start_ids.push_back({0, 0});
        } else {
          if (it->second->getType() != RedisStoreValue::STREAM) {
            throw std::runtime_error("Invalid key: " + store_key +
                                     " provided for stream");
          }
          auto &stream_val = static_cast<StreamValue &>(*(it->second));
          base_start_ids.push_back(stream_val.getTopEntry());
        }
      }
    }
  }

  fillEntries();

  bool timed_out = false;
  if (timeout_ms && ret.empty()) {
    std::vector<std::unique_ptr<FifoBlockingQueue::WaitToken>> wait_tokens;
    {
      auto queues = blockingQueues();
      for (const auto &store_key : store_keys) {
        wait_tokens.push_back(queues.getQueue(store_key).create_wait_token());
      }
    }

    std::mutex notification_mutex;
    std::condition_variable notification_cv;
    std::atomic<bool> any_notified{false};
    std::vector<std::thread> waiting_threads;

    for (auto &token : wait_tokens) {
      waiting_threads.emplace_back([&]() {
        if (*timeout_ms == 0) {
          token->wait();
        } else {
          token->wait_for(std::chrono::milliseconds(*timeout_ms));
        }

        {
          std::lock_guard<std::mutex> lock(notification_mutex);
          any_notified.store(true);
        }
        notification_cv.notify_one();
      });
    }

    if (*timeout_ms == 0) {
      while (ret.empty()) {
        std::unique_lock<std::mutex> lock(notification_mutex);
        notification_cv.wait(lock, [&]() { return any_notified.load(); });

        fillEntries();

        if (!ret.empty()) {
          break;
        }

        any_notified.store(false);
      }
    } else {
      auto timeout_duration = std::chrono::milliseconds(*timeout_ms);
      auto start_time = std::chrono::steady_clock::now();

      while (ret.empty()) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto remaining =
            timeout_duration -
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

        if (remaining <= std::chrono::milliseconds(0)) {
          std::cerr << "Deadline of " << *timeout_ms << " ms reached"
                    << std::endl;
          timed_out = true;
          break;
        }

        std::unique_lock<std::mutex> lock(notification_mutex);
        if (notification_cv.wait_for(lock, remaining,
                                     [&]() { return any_notified.load(); })) {
          fillEntries();

          if (!ret.empty()) {
            break;
          }

          any_notified.store(false);
        } else {
          std::cerr << "Deadline of " << *timeout_ms << " ms reached"
                    << std::endl;
          timed_out = true;
          break;
        }
      }
    }

    for (auto &thread : waiting_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  return {timed_out, ret};
}

std::vector<std::string> RedisStore::getKeys() const {
  std::vector<std::string> keys;
  auto store = readStore();
  std::transform(store->begin(), store->end(), std::back_inserter(keys),
                 [](const auto &pair) { return pair.first; });
  return keys;
}

void RedisStore::notifyBlockingClients(const std::string &key) const {
  auto queues = blockingQueues();
  auto it = queues->find(key);
  if (it != queues->end()) {
    it->second.notify_one();
  }
}

std::size_t RedisStore::addMemberToSet(const std::string &key, double score,
                                       const std::string &member) {
  auto store = writeStore();
  auto it = store->find(key);

  std::size_t ret = 1;
  if (it == store->end()) {
    auto val = std::make_unique<SetValue>();
    store->insert_or_assign(key, std::move(val));
  } else {
    auto &set = static_cast<SetValue &>(*it->second);
    // TODO: handle existing member
    set.addMember(score, member);
  }
  notifyBlockingClients(key);
  return ret;
}

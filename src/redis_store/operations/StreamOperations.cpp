#include "redis_store/RedisStore.hpp"
#include "redis_store/values/StreamValue.hpp"
#include "utils/genericUtils.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {
uint64_t nowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

uint64_t generateSequenceNumber(uint64_t timestamp) {
  return timestamp == 0 ? 1 : 0;
}
} // namespace

StreamValue::StreamEntryId
RedisStore::computeEntryIdForExistingStream(StreamValue &stream,
                                            std::optional<uint64_t> timestamp,
                                            std::optional<uint64_t> sequence) {
  auto [top_ts, top_seq] = stream.getTopEntry();

  if (timestamp) {
    uint64_t now = nowMs();
    if (*timestamp > now) {
      throw std::runtime_error("The timestamp in ID is from the future");
    }
    if (*timestamp < top_ts ||
        (*timestamp == top_ts && sequence && *sequence <= top_seq)) {
      throw std::runtime_error("The ID specified in XADD is equal or smaller "
                               "than the target stream top item");
    }
    if (*timestamp == top_ts && !sequence) {
      return {top_ts, top_seq + 1};
    }
    return {*timestamp, sequence.value_or(generateSequenceNumber(*timestamp))};
  }

  if (sequence) {
    throw std::runtime_error("Unsupported format: sequence without timestamp");
  }
  uint64_t ts_now = nowMs();
  if (ts_now < top_ts) {
    throw std::logic_error(
        "Generated timestamp is smaller than top entry's timestamp");
  }
  uint64_t seq =
      (ts_now == top_ts ? top_seq + 1 : generateSequenceNumber(ts_now));
  return {ts_now, seq};
}

StreamValue::StreamEntryId
RedisStore::addStreamEntry(const std::string &key, const std::string &entry_id,
                           StreamValue::StreamEntry entry) {
  StreamValue::StreamEntryId saved_entry_id;

  if (entry_id == "*") {
    auto ts_now = nowMs();
    uint64_t seq = generateSequenceNumber(ts_now);
    saved_entry_id = {ts_now, seq};

    auto store = writeStore();
    auto [stream_ptr, inserted] = store->try_emplace(key, StreamValue());
    std::get<StreamValue>(stream_ptr->second)
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
  auto itr = store->find(key);

  if (itr == store->end()) {
    auto stream_val = StreamValue();
    uint64_t ts_now = timestamp.value_or(nowMs());
    uint64_t seq = sequence.value_or(generateSequenceNumber(ts_now));
    saved_entry_id = {ts_now, seq};
    stream_val.addEntry(saved_entry_id, std::move(entry));
    (*store)[key] = stream_val;
  } else {
    auto *stream = std::get_if<StreamValue>(&itr->second);
    if (stream == nullptr) {
      throw std::runtime_error("Provided key doesn't contain STREAM value");
    }
    saved_entry_id =
        computeEntryIdForExistingStream(*stream, timestamp, sequence);
    stream->addEntry(saved_entry_id, std::move(entry));
  }

  notifyBlockingClients(key);
  return saved_entry_id;
}

std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
RedisStore::getStreamEntriesInRange(const std::string &store_key,
                                    const std::string &entry_id_start,
                                    const std::string &entry_id_end) const {
  auto store = readStore();
  auto itr = store->find(store_key);

  if (itr == store->end()) {
    throw std::runtime_error("Invalid key provided for stream");
  }

  const auto *stream_val = std::get_if<StreamValue>(&itr->second);
  if (stream_val == nullptr) {
    throw std::runtime_error("Invalid key provided for stream");
  }

  StreamValue::StreamIterator it1;
  StreamValue::StreamIterator it2;
  if (entry_id_start == "-") {
    it1 = stream_val->begin();
  } else {
    it1 = stream_val->lowerBound(parseStreamEntryId(entry_id_start));
  }
  if (entry_id_end == "+") {
    it2 = stream_val->end();
  } else {
    it2 = stream_val->upperBound(parseStreamEntryId(entry_id_end));
  }

  std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
      ret;
  while (it1 != it2) {
    ret.emplace_back(it1->first, it1->second);
    ++it1;
  }
  return ret;
}

bool RedisStore::waitForStreamEntries(
    const std::vector<std::string> &store_keys, uint64_t timeout_ms,
    const std::function<void()> &fill_entries,
    const std::function<bool()> &has_entries) const {

  std::vector<std::unique_ptr<FifoBlockingQueue::WaitToken>> wait_tokens;
  for (const auto &store_key : store_keys) {
    auto queue = getBlockingQueue(store_key);
    wait_tokens.push_back(queue->create_wait_token());
  }

  std::mutex notification_mutex;
  std::condition_variable notification_cv;
  std::atomic<bool> any_notified{false};
  std::vector<std::thread> waiting_threads;
  waiting_threads.reserve(wait_tokens.size());

  for (auto &token : wait_tokens) {
    waiting_threads.emplace_back([&]() {
      if (timeout_ms == 0) {
        token->wait();
      } else {
        token->wait_for(std::chrono::milliseconds(timeout_ms));
      }
      {
        std::lock_guard<std::mutex> lock(notification_mutex);
        any_notified.store(true);
      }
      notification_cv.notify_one();
    });
  }

  bool timed_out = false;
  if (timeout_ms == 0) {
    while (!has_entries()) {
      std::unique_lock<std::mutex> lock(notification_mutex);
      notification_cv.wait(lock, [&]() { return any_notified.load(); });
      fill_entries();
      any_notified.store(false);
    }
  } else {
    auto timeout_duration = std::chrono::milliseconds(timeout_ms);
    auto start_time = std::chrono::steady_clock::now();

    while (!has_entries()) {
      auto elapsed = std::chrono::steady_clock::now() - start_time;
      auto remaining =
          timeout_duration -
          std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

      if (remaining <= std::chrono::milliseconds(0)) {
        std::cerr << "Deadline of " << timeout_ms << " ms reached" << '\n';
        timed_out = true;
        break;
      }

      std::unique_lock<std::mutex> lock(notification_mutex);
      if (notification_cv.wait_for(lock, remaining,
                                   [&]() { return any_notified.load(); })) {
        fill_entries();
        any_notified.store(false);
      } else {
        std::cerr << "Deadline of " << timeout_ms << " ms reached" << '\n';
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

  return timed_out;
}

void RedisStore::fillStreamEntries(
    const std::vector<std::string> &store_keys,
    const std::vector<std::array<uint64_t, 2>> &base_start_ids,
    StreamEntriesMap &result) const {
  auto store = readStore();
  for (std::size_t idx = 0; idx < store_keys.size(); ++idx) {
    const auto &store_key = store_keys.at(idx);
    auto itr = store->find(store_key);
    if (itr == store->end()) {
      continue;
    }
    const auto *stream_val = std::get_if<StreamValue>(&itr->second);
    if (stream_val == nullptr) {
      throw std::runtime_error("Invalid key: " + store_key +
                               " provided for stream");
    }
    std::array<uint64_t, 2> start_id = base_start_ids.at(idx);

    std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
        entries;
    auto it1 = stream_val->upperBound(start_id);
    while (it1 != stream_val->end()) {
      entries.emplace_back(it1->first, it1->second);
      ++it1;
    }

    if (!entries.empty()) {
      result[store_key] = entries;
    }
  }
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
  StreamEntriesMap ret;
  std::vector<std::array<uint64_t, 2>> base_start_ids;

  {
    auto store = readStore();
    for (std::size_t idx = 0; idx < nkeys; ++idx) {
      const auto &entry_id_start = entry_ids_start.at(idx);
      if (entry_id_start != "$") {
        base_start_ids.push_back(parseStreamEntryId(entry_id_start));
        continue;
      }
      const auto &store_key = store_keys.at(idx);
      auto itr = store->find(store_key);
      if (itr == store->end()) {
        base_start_ids.push_back({0, 0});
        continue;
      }
      const auto *stream_val = std::get_if<StreamValue>(&itr->second);
      if (stream_val == nullptr) {
        throw std::runtime_error("Invalid key: " + store_key +
                                 " provided for stream");
      }
      base_start_ids.push_back(stream_val->getTopEntry());
    }
  }

  fillStreamEntries(store_keys, base_start_ids, ret);

  bool timed_out = false;
  if (timeout_ms && ret.empty()) {
    auto fill_fn = [&]() {
      fillStreamEntries(store_keys, base_start_ids, ret);
    };
    timed_out = waitForStreamEntries(store_keys, *timeout_ms, fill_fn,
                                     [&]() { return !ret.empty(); });
  }

  return {timed_out, ret};
}

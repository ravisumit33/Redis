#include "RedisStore.hpp"
#include "utils.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>

StreamValue::StreamEntryId StreamValue::getTopEntry() const {
  if (mStreams.empty()) {
    throw std::logic_error("Stream should not be empty");
  }
  auto it = mStreams.rbegin();
  return it->first;
}

void RedisStore::setString(const std::string &key, const std::string &value,
                           std::optional<std::chrono::milliseconds> exp) {
  std::lock_guard<std::shared_mutex> lock(mMutex);
  std::optional<std::chrono::steady_clock::time_point> expiry = std::nullopt;
  if (exp) {
    expiry = std::chrono::steady_clock::now() + exp.value();
  }
  auto val = std::make_unique<StringValue>(value, expiry);
  mStore.insert_or_assign(key, std::move(val));
  m_cv.notify_all();
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

    std::lock_guard<std::shared_mutex> lock(mMutex);
    auto [stream_ptr, inserted] =
        mStore.try_emplace(key, std::make_unique<StreamValue>());
    static_cast<StreamValue &>(*stream_ptr->second)
        .addEntry(saved_entry_id, std::move(entry));
    m_cv.notify_all();
    return saved_entry_id;
  }

  auto [timestamp, sequence] = parsePartialStreamEntryId(entry_id);

  if (timestamp == 0 && sequence == 0) {
    throw std::runtime_error(
        "The ID specified in XADD must be greater than 0-0");
  }

  std::lock_guard<std::shared_mutex> lock(mMutex);
  auto it = mStore.find(key);

  if (it == mStore.end()) {
    auto stream_ptr = std::make_unique<StreamValue>();
    auto &stream = *stream_ptr;
    uint64_t ts = timestamp.value_or(nowMs());
    uint64_t seq = sequence.value_or(generateSequenceNumber(ts));
    saved_entry_id = {ts, seq};
    stream.addEntry(saved_entry_id, std::move(entry));
    mStore[key] = std::move(stream_ptr);
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

  m_cv.notify_all();
  return saved_entry_id;
}

std::optional<std::unique_ptr<RedisStoreValue>>
RedisStore::get(const std::string &key) const {
  std::shared_lock<std::shared_mutex> lock(mMutex);
  auto it = mStore.find(key);
  if (it == mStore.end()) {
    return std::nullopt;
  }

  auto &val = it->second;
  if (val->hasExpired()) {
    return std::nullopt;
  }

  return val->clone();
}

bool RedisStore::keyExists(const std::string &key) const {
  std::shared_lock<std::shared_mutex> lock(mMutex);
  return mStore.contains(key);
}

std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
RedisStore::getStreamEntriesInRange(const std::string &store_key,
                                    const std::string &entry_id_start,
                                    const std::string &entry_id_end) const {
  std::shared_lock<std::shared_mutex> lock(mMutex);

  auto it = mStore.find(store_key);

  if (it == mStore.end() || it->second->getType() != RedisStoreValue::STREAM) {
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

  std::optional<std::chrono::steady_clock::time_point> deadline;
  if (timeout_ms) {
    deadline = std::chrono::steady_clock::now() +
               std::chrono::milliseconds(*timeout_ms);
  }

  std::unordered_map<std::string,
                     std::vector<std::pair<StreamValue::StreamEntryId,
                                           StreamValue::StreamEntry>>>
      ret;

  auto stream_view = std::views::zip(store_keys, entry_ids_start) |
                     std::views::transform([this](auto pair) {
                       const auto &[store_key, entry_id_start] = pair;
                       auto it = mStore.find(store_key);

                       if (it == mStore.end() ||
                           it->second->getType() != RedisStoreValue::STREAM) {
                         throw std::runtime_error("Invalid key: " + store_key +
                                                  " provided for stream");
                       }
                       auto &stream_val =
                           static_cast<StreamValue &>(*(it->second));
                       std::array<uint64_t, 2> start_id{0, 0};
                       if (entry_id_start != "$") {
                         start_id = parseStreamEntryId(entry_id_start);
                       } else if (!stream_val.empty()) {
                         start_id = stream_val.getTopEntry();
                       }
                       return std::tuple{start_id, std::cref(stream_val),
                                         std::cref(store_key)};
                     });

  auto stream_iterable = std::vector(stream_view.begin(), stream_view.end());

  auto fillEntries = [&]() {
    for (const auto &[start_id, stream_val_ref, store_key_ref] :
         stream_iterable) {
      std::vector<
          std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
          entries;
      const auto &stream_val = stream_val_ref.get();
      const auto &store_key = store_key_ref.get();
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

  std::unique_lock<std::shared_mutex> lock(mMutex);
  fillEntries();

  bool timed_out = false;
  if (timeout_ms && ret.empty()) {
    if (*timeout_ms == 0) {
      while (ret.empty()) {
        m_cv.wait(lock);
        fillEntries();
      }
    } else {
      while (ret.empty()) {
        if (m_cv.wait_until(lock, *deadline) == std::cv_status::timeout) {
          std::cout << "Dealine of " << *timeout_ms << " ms reached"
                    << std::endl;
          timed_out = true;
          break;
        }
        fillEntries();
      }
    }
  }

  return {timed_out, ret};
}

std::vector<std::string> RedisStore::getKeys() const {
  std::vector<std::string> keys;
  std::transform(mStore.begin(), mStore.end(), std::back_inserter(keys),
                 [](const auto &pair) { return pair.first; });
  return keys;
}

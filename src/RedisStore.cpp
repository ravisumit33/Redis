#include "RedisStore.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

StreamValue::StreamEntryId StreamValue::getTopEntry() const {
  if (mStreams.empty()) {
    throw std::logic_error("Stream should not be empty");
  }
  auto it = mStreams.rbegin();
  return it->first;
}

void RedisStore::setString(const std::string &key, const std::string &value,
                           std::optional<std::chrono::milliseconds> exp) {
  std::lock_guard<std::mutex> lock(mMutex);
  std::optional<std::chrono::steady_clock::time_point> expiry = std::nullopt;
  if (exp) {
    expiry = std::chrono::steady_clock::now() + exp.value();
  }
  auto val = std::make_unique<StringValue>(value, expiry);
  mStore.insert_or_assign(key, std::move(val));
}

StreamValue::StreamEntryId
RedisStore::addStreamEntry(const std::string &key, const std::string &entry_id,
                           StreamValue::StreamEntry entry) {

  using EntryIdPart = std::variant<uint64_t, std::string>;
  auto parseEntryId =
      [](const std::string &entry_id) -> std::array<EntryIdPart, 2> {
    std::size_t hyphen_pos = entry_id.find("-");
    if (hyphen_pos == std::string::npos) {
      throw std::runtime_error("Invalid entry_id for stream");
    }

    auto parsePart = [](const std::string &s) -> EntryIdPart {
      try {
        return std::stoull(s);
      } catch (...) {
        return s;
      }
    };

    return {parsePart(entry_id.substr(0, hyphen_pos)),
            parsePart(entry_id.substr(hyphen_pos + 1))};
  };

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

    std::lock_guard<std::mutex> lock(mMutex);
    auto [stream_ptr, inserted] =
        mStore.try_emplace(key, std::make_unique<StreamValue>());
    static_cast<StreamValue &>(*stream_ptr->second)
        .addEntry(saved_entry_id, std::move(entry));
    return saved_entry_id;
  }

  auto [part1, part2] = parseEntryId(entry_id);
  std::optional<uint64_t> timestamp, sequence;

  auto extractPart = [](const EntryIdPart &part,
                        std::optional<uint64_t> &target, const char *which) {
    if (auto p = std::get_if<uint64_t>(&part)) {
      target = *p;
    } else if (auto str = std::get_if<std::string>(&part)) {
      if (*str != "*") {
        throw std::runtime_error(
            std::string("Invalid entry_id component in XADD: ") + which);
      }
    } else {
      throw std::runtime_error("Unhandled variant type in entry_id part");
    }
  };

  extractPart(part1, timestamp, "timestamp");
  extractPart(part2, sequence, "sequence");

  if (timestamp == 0 && sequence == 0) {
    throw std::runtime_error(
        "The ID specified in XADD must be greater than 0-0");
  }

  std::lock_guard<std::mutex> lock(mMutex);
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
      saved_entry_id = {ts, generateSequenceNumber(ts)};
    }

    stream.addEntry(saved_entry_id, std::move(entry));
  }

  return saved_entry_id;
}

std::optional<std::unique_ptr<RedisStoreValue>>
RedisStore::get(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mMutex);
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
  std::lock_guard<std::mutex> lock(mMutex);
  return mStore.contains(key);
}

#include "RedisStore.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

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

void RedisStore::addStreamEntry(const std::string &key,
                                const StreamValue::StreamEntryId &entry_id,
                                StreamValue::StreamEntry entry) {

  auto getEntryIdParts = [](const StreamValue::StreamEntryId &entry_id)
      -> std::array<uint64_t, 2> {
    std::size_t hyphen_pos = entry_id.find("-");
    if (hyphen_pos == std::string::npos) {
      throw std::runtime_error("Invalid entry_id for stream");
    }
    auto id_part1 = std::stoull(entry_id.substr(0, hyphen_pos));
    auto id_part2 = std::stoull(entry_id.substr(hyphen_pos + 1));
    return {id_part1, id_part2};
  };

  auto [entry_id_part1, entry_id_part2] = getEntryIdParts(entry_id);

  std::lock_guard<std::mutex> lock(mMutex);
  auto it = mStore.find(key);

  if (entry_id_part1 == 0 && entry_id_part2 == 0) {
    throw std::runtime_error(
        "The ID specified in XADD must be greater than 0-0");
  }

  if (it == mStore.end()) {
    auto val = std::make_unique<StreamValue>();
    val->addEntry(entry_id, std::move(entry));
    mStore[key] = std::move(val);
  } else {
    auto &stream = static_cast<StreamValue &>(*it->second);
    auto top_entry_id = stream.getTopEntry();
    auto [top_entry_id_part1, top_entry_id_part2] =
        getEntryIdParts(top_entry_id);

    if (entry_id_part1 < top_entry_id_part1) {
      throw std::runtime_error("The ID specified in XADD is equal or smaller "
                               "than the target stream top item");
    }

    if (entry_id_part1 == top_entry_id_part1 &&
        entry_id_part2 <= top_entry_id_part2) {
      throw std::runtime_error("The ID specified in XADD is equal or smaller "
                               "than the target stream top item");
    }
    stream.addEntry(entry_id, std::move(entry));
  }
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

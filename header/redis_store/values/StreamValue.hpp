#pragma once

#include "redis_store/values/RedisStoreValue.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>

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

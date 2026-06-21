#pragma once

#include "ExpiryInfo.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

class StreamValue {
public:
  using StreamEntry = std::unordered_map<std::string, std::string>;
  using StreamEntryId = std::array<uint64_t, 2>;
  using StreamMap = std::map<StreamEntryId, StreamEntry>;
  using StreamIterator = StreamMap::const_iterator;

  StreamValue() = default;

  StreamIterator begin() const { return m_streams.begin(); }

  StreamIterator end() const { return m_streams.end(); }

  bool empty() const { return m_streams.empty(); }

  void addEntry(const StreamEntryId &stream_id, StreamEntry stream_entry) {
    m_streams[stream_id].merge(stream_entry);
  }

  StreamEntryId getTopEntry() const;

  StreamIterator lowerBound(const StreamEntryId &entry_id) const {
    return m_streams.lower_bound(entry_id);
  }

  StreamIterator upperBound(const StreamEntryId &entry_id) const {
    return m_streams.upper_bound(entry_id);
  }

  const ExpiryInfo &getExpiryInfo() const { return m_expiry_info; }

private:
  StreamMap m_streams;
  ExpiryInfo m_expiry_info;
};

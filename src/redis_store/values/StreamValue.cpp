#include "redis_store/values/StreamValue.hpp"
#include <stdexcept>

StreamValue::StreamEntryId StreamValue::getTopEntry() const {
  if (m_streams.empty()) {
    throw std::logic_error("Stream should not be empty");
  }
  auto itr = m_streams.rbegin();
  return itr->first;
}

#include "redis_store/values/StreamValue.hpp"
#include <stdexcept>

StreamValue::StreamEntryId StreamValue::getTopEntry() const {
  if (mStreams.empty()) {
    throw std::logic_error("Stream should not be empty");
  }
  auto it = mStreams.rbegin();
  return it->first;
}

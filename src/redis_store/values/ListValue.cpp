#include "redis_store/values/ListValue.hpp"
#include <algorithm>

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

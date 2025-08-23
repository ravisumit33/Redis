#include "redis_store/values/SetValue.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>

bool SetValue::addMember(double score, const std::string &member) {
  auto it = m_score_map.find(member);
  bool inserted = true;
  if (it != m_score_map.end()) {
    m_set.erase({it->second, member});
    inserted = false;
  }
  m_score_map[member] = score;
  m_set.insert({score, member});
  return inserted;
}

unsigned int SetValue::getRank(const std::string &member) {
  double score = m_score_map.at(member);
  auto it = m_set.find({score, member});
  // This can be optimized if we use better data structure
  return std::distance(m_set.begin(), it);
}

std::vector<std::string> SetValue::getElementsInRange(int start_idx,
                                                      int end_idx) {
  if (empty()) {
    return {};
  }

  std::size_t num_elements = m_set.size();
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

  std::vector<std::string> memberInRange;
  std::transform(std::next(m_set.begin(), start_idx),
                 std::next(m_set.begin(), end_idx + 1),
                 std::back_inserter(memberInRange),
                 [](const auto &p) { return p.second; });
  return memberInRange;
}

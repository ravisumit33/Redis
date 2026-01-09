#include "redis_store/values/SetValue.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

bool SetValue::addMember(double score, const std::string &member) {
  auto itr = m_score_map.find(member);
  bool inserted = true;
  if (itr != m_score_map.end()) {
    m_sorted_set.erase({itr->second, member});
    inserted = false;
  }
  m_score_map[member] = score;
  m_sorted_set.insert({score, member});
  return inserted;
}

unsigned int SetValue::getRank(const std::string &member) const {
  double score = m_score_map.at(member);
  auto itr = m_sorted_set.find({score, member});
  // This can be optimized if we use better data structure
  return std::distance(m_sorted_set.begin(), itr);
}

std::vector<std::string> SetValue::getElementsInRange(int start_idx,
                                                      int end_idx) const {
  if (empty()) {
    return {};
  }

  std::size_t num_elements = m_sorted_set.size();
  if (start_idx < 0) {
    start_idx += static_cast<int>(num_elements);
  }
  if (end_idx < 0) {
    end_idx += static_cast<int>(num_elements);
  }
  start_idx = std::max(start_idx, 0);
  end_idx = std::min<int>(end_idx, static_cast<int>(num_elements) - 1);
  if (start_idx > end_idx) {
    return {};
  }

  std::vector<std::string> memberInRange;
  std::transform(std::next(m_sorted_set.begin(), start_idx),
                 std::next(m_sorted_set.begin(), end_idx + 1),
                 std::back_inserter(memberInRange),
                 [](const auto &pair) { return pair.second; });
  return memberInRange;
}

std::size_t SetValue::erase(const std::string &member) {
  auto itr = m_score_map.find(member);
  if (itr == m_score_map.end()) {
    return 0;
  }
  m_sorted_set.erase({itr->second, member});
  m_score_map.erase(itr);
  return 1;
}

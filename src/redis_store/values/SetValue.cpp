#include "redis_store/values/SetValue.hpp"
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

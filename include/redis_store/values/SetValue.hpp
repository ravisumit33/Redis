#pragma once

#include "ExpiryInfo.hpp"
#include <cstddef>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class SetValue {
public:
  SetValue() = default;

  bool addMember(double score, const std::string &member);

  bool empty() const { return m_score_map.empty(); }

  unsigned int getRank(const std::string &member) const;

  double getScore(const std::string &member) const {
    return m_score_map.at(member);
  }

  std::vector<std::string> getElementsInRange(int start_idx, int end_idx) const;

  std::size_t size() const { return m_sorted_set.size(); }

  std::size_t erase(const std::string &member);

  const ExpiryInfo &getExpiryInfo() const { return m_expiry_info; }

private:
  std::unordered_map<std::string, double> m_score_map;
  std::set<std::pair<double, std::string>> m_sorted_set;
  ExpiryInfo m_expiry_info;
};

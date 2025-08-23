#pragma once

#include "redis_store/values/RedisStoreValue.hpp"
#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

class SetValue : public RedisStoreValue {
public:
  SetValue() : RedisStoreValue(SET) {}

  bool addMember(double score, const std::string &member) {
    auto it = m_score_map.find(member);
    bool inserted = true;
    if (it != m_score_map.end()) {
      m_set.erase(member);
      inserted = false;
    }
    m_score_map[member] = score;
    m_set.insert(member);
    return inserted;
  }

  std::size_t size() const { return m_set.size(); }

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<SetValue>(*this);
  };

private:
  using score_map_t = std::unordered_map<std::string, double>;

  score_map_t m_score_map;

  struct Comparator {
    const score_map_t &score_map;

    Comparator(const score_map_t &map_ref) : score_map(map_ref) {}

    bool operator()(const auto &lhs, const auto &rhs) const {
      return score_map.at(lhs) < score_map.at(rhs);
    }
  };

  std::set<std::string, Comparator> m_set{Comparator(m_score_map)};
};

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

  bool addMember(double score, const std::string &member);

  unsigned int getRank(const std::string &member);

  std::size_t size() const { return m_set.size(); }

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<SetValue>(*this);
  };

private:
  std::unordered_map<std::string, double> m_score_map;
  std::set<std::pair<double, std::string>> m_set;
};

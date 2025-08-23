#pragma once

#include "redis_store/values/RedisStoreValue.hpp"
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class ListValue : public RedisStoreValue {
public:
  ListValue() : RedisStoreValue(LIST) {}

  ListValue(const std::vector<std::string> &elements)
      : RedisStoreValue(LIST), m_elements(elements.begin(), elements.end()) {}

  ListValue &insertAtBegin(const std::vector<std::string> &v) {
    m_elements.insert(m_elements.begin(), v.begin(), v.end());
    return *this;
  }

  ListValue &insertAtEnd(const std::vector<std::string> &v) {
    m_elements.insert(m_elements.end(), v.begin(), v.end());
    return *this;
  }

  bool empty() const { return m_elements.empty(); }

  std::size_t size() const { return m_elements.size(); }

  std::string pop_front() {
    std::string front_el = m_elements.front();
    m_elements.pop_front();
    return front_el;
  }

  std::vector<std::string> getElementsInRange(int start_idx, int end_idx);

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<ListValue>(*this);
  };

private:
  std::deque<std::string> m_elements;
};

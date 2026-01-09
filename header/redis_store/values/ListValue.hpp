#pragma once

#include "ExpiryInfo.hpp"
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

class ListValue {
public:
  ListValue() = default;

  explicit ListValue(const std::vector<std::string> &vec)
      : m_elements(vec.begin(), vec.end()) {}

  ListValue &insertAtBegin(const std::vector<std::string> &vec) {
    m_elements.insert(m_elements.begin(), vec.begin(), vec.end());
    return *this;
  }

  ListValue &insertAtEnd(const std::vector<std::string> &vec) {
    m_elements.insert(m_elements.end(), vec.begin(), vec.end());
    return *this;
  }

  bool empty() const { return m_elements.empty(); }

  std::size_t size() const { return m_elements.size(); }

  std::string pop_front() {
    std::string front_el = m_elements.front();
    m_elements.pop_front();
    return front_el;
  }

  std::vector<std::string> getElementsInRange(int start_idx, int end_idx) const;

  const ExpiryInfo &getExpiryInfo() const { return m_expiry_info; }

private:
  std::deque<std::string> m_elements;
  ExpiryInfo m_expiry_info;
};

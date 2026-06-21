#pragma once

#include <chrono>
#include <optional>

class ExpiryInfo {
public:
  ExpiryInfo() = default;

  explicit ExpiryInfo(std::optional<std::chrono::system_clock::time_point> exp)
      : m_expiry(exp) {}

  bool hasExpired() const {
    if (!m_expiry) {
      return false;
    }
    return std::chrono::system_clock::now() >= m_expiry.value();
  }

private:
  std::optional<std::chrono::system_clock::time_point> m_expiry;
};

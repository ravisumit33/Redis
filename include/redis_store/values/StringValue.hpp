#pragma once

#include "ExpiryInfo.hpp"
#include <chrono>
#include <optional>
#include <string>

class StringValue {
public:
  StringValue() = default;

  StringValue(
      std::string val,
      std::optional<std::chrono::system_clock::time_point> exp = std::nullopt)
      : m_value(std::move(val)), m_expiry_info(exp) {}

  const std::string &getValue() const { return m_value; }

  const ExpiryInfo &getExpiryInfo() const { return m_expiry_info; }

private:
  std::string m_value;
  ExpiryInfo m_expiry_info;
};

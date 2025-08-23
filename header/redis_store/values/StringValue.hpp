#pragma once

#include "redis_store/values/RedisStoreValue.hpp"
#include <memory>
#include <optional>
#include <string>

class StringValue : public RedisStoreValue {
public:
  StringValue(const std::string &val,
              std::optional<std::chrono::system_clock::time_point> exp)
      : RedisStoreValue(STRING, exp), mValue(val) {}

  std::string getValue() const { return mValue; }

  virtual std::unique_ptr<RedisStoreValue> clone() const override {
    return std::make_unique<StringValue>(*this);
  };

private:
  std::string mValue;
};

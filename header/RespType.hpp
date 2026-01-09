#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

inline constexpr std::string_view kCrlf = "\r\n";

enum class RespType : std::uint8_t {
  STRING,
  INTEGER,
  ARRAY,
  BULK_STRING,
  ERROR
};

class RespString {
public:
  explicit RespString(std::string val) : m_value(std::move(val)) {}

  const std::string &getValue() const { return m_value; }

  std::string serialize() const {
    return std::string("+") + m_value + std::string(kCrlf);
  }

private:
  std::string m_value;
};

class RespInt {
public:
  explicit RespInt(int64_t val) : m_value(val) {}

  int64_t getValue() const { return m_value; }

  std::string serialize() const {
    return std::string(":") + std::to_string(m_value) + std::string(kCrlf);
  }

private:
  int64_t m_value;
};

class RespBulkString {
public:
  RespBulkString() : m_value(std::nullopt), m_lastCrlf(true) {}

  explicit RespBulkString(std::string val, bool lastCrlf = true)
      : m_value(std::move(val)), m_lastCrlf(lastCrlf) {}

  const std::string &getValue() const { return *m_value; }

  bool isNull() const { return !m_value.has_value(); }

  std::string serialize() const {
    if (!m_value) {
      return std::string("$-1") + std::string(kCrlf);
    }
    auto resp = std::string("$") + std::to_string(m_value->length()) +
                std::string(kCrlf) + *m_value;
    if (m_lastCrlf) {
      resp += std::string(kCrlf);
    }
    return resp;
  }

private:
  std::optional<std::string> m_value;
  bool m_lastCrlf;
};

class RespError {
public:
  explicit RespError(std::string val) : m_value(std::move(val)) {}

  const std::string &getValue() const { return m_value; }

  std::string serialize() const {
    return std::string("-") + "ERR " + m_value + std::string(kCrlf);
  }

private:
  std::string m_value;
};

class RespValue;

class RespArray {
public:
  RespArray() = default;

  RespArray(const RespArray &other);

  RespArray(RespArray &&other) noexcept = default;

  RespArray &operator=(const RespArray &other);

  RespArray &operator=(RespArray &&other) noexcept = default;

  ~RespArray() = default;

  RespArray &add(RespValue item);

  const RespValue &at(size_t idx) const;

  RespValue &at(size_t idx);

  size_t size() const { return m_elements.size(); }

  bool empty() const { return m_elements.empty(); }

  std::vector<RespValue> release();

  std::string serialize() const;

private:
  std::vector<std::unique_ptr<RespValue>> m_elements;
};

using RespValueVariant =
    std::variant<RespString, RespInt, RespBulkString, RespError, RespArray>;

class RespValue {
public:
  RespValue(RespString val) : m_data(std::move(val)) {}
  RespValue(RespInt val) : m_data(val) {}
  RespValue(RespBulkString val) : m_data(std::move(val)) {}
  RespValue(RespError val) : m_data(std::move(val)) {}
  RespValue(RespArray val) : m_data(std::move(val)) {}

  RespValue(const RespValue &other) = default;
  RespValue &operator=(const RespValue &other) = default;
  RespValue(RespValue &&other) noexcept = default;
  RespValue &operator=(RespValue &&other) noexcept = default;
  ~RespValue() = default;

  std::string serialize() const {
    return std::visit([](const auto &val) { return val.serialize(); }, m_data);
  }

  RespValue clone() const { return *this; }

  RespType getType() const {
    return std::visit(
        [](const auto &val) -> RespType {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, RespString>) {
            return RespType::STRING;
          } else if constexpr (std::is_same_v<T, RespInt>) {
            return RespType::INTEGER;
          } else if constexpr (std::is_same_v<T, RespArray>) {
            return RespType::ARRAY;
          } else if constexpr (std::is_same_v<T, RespBulkString>) {
            return RespType::BULK_STRING;
          } else if constexpr (std::is_same_v<T, RespError>) {
            return RespType::ERROR;
          }
        },
        m_data);
  }

  template <typename T> const T *getIf() const {
    return std::get_if<T>(&m_data);
  }

  template <typename T> T *getIf() { return std::get_if<T>(&m_data); }

  template <typename T> const T &get() const { return std::get<T>(m_data); }

  template <typename T> T &get() { return std::get<T>(m_data); }

  const RespValueVariant &data() const { return m_data; }
  RespValueVariant &data() { return m_data; }

private:
  RespValueVariant m_data;
};

inline RespArray::RespArray(const RespArray &other) {
  m_elements.reserve(other.m_elements.size());
  for (const auto &elem : other.m_elements) {
    m_elements.push_back(std::make_unique<RespValue>(*elem));
  }
}

inline RespArray &RespArray::operator=(const RespArray &other) {
  if (this != &other) {
    m_elements.clear();
    m_elements.reserve(other.m_elements.size());
    for (const auto &elem : other.m_elements) {
      m_elements.push_back(std::make_unique<RespValue>(*elem));
    }
  }
  return *this;
}

inline RespArray &RespArray::add(RespValue item) {
  m_elements.push_back(std::make_unique<RespValue>(std::move(item)));
  return *this;
}

inline const RespValue &RespArray::at(size_t idx) const {
  return *m_elements.at(idx);
}

inline RespValue &RespArray::at(size_t idx) { return *m_elements.at(idx); }

inline std::vector<RespValue> RespArray::release() {
  std::vector<RespValue> result;
  result.reserve(m_elements.size());
  for (auto &elem : m_elements) {
    result.push_back(std::move(*elem));
  }
  m_elements.clear();
  return result;
}

inline std::string RespArray::serialize() const {
  std::string serialized_string("*");
  serialized_string += std::to_string(m_elements.size()) + std::string(kCrlf);
  for (const auto &element : m_elements) {
    serialized_string += element->serialize();
  }
  return serialized_string;
}

inline const std::string &getStringValue(const RespValue &value) {
  if (const auto *bulk = value.getIf<RespBulkString>()) {
    return bulk->getValue();
  }
  if (const auto *str = value.getIf<RespString>()) {
    return str->getValue();
  }
  throw std::runtime_error("Value is not a string type");
}

inline int64_t getIntValue(const RespValue &value) {
  return value.get<RespInt>().getValue();
}

#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class RespType {
public:
  enum Type { STRING, INTEGER, ARRAY, BULK_STRING, ERROR };

  RespType(Type t) : type(t) {}

  virtual ~RespType() = default;

  virtual std::unique_ptr<RespType> clone() = 0;

  Type getType() const { return type; }

  virtual std::string serialize() = 0;

private:
  Type type;

protected:
  std::string CRLF = "\r\n";
};

class RespString : public RespType {
public:
  RespString(std::string val) : RespType(STRING), m_value(std::move(val)) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespString>(*this);
  }

  std::string getValue() const { return m_value; }

  virtual std::string serialize() override {
    return std::string("+") + getValue() + CRLF;
  }

private:
  std::string m_value;
};

class RespInt : public RespType {
public:
  RespInt(int64_t val) : RespType(INTEGER), m_value(val) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespInt>(*this);
  }

  int64_t getValue() const { return m_value; }

  virtual std::string serialize() override {
    return std::string(":") + std::to_string(getValue()) + CRLF;
  }

private:
  int64_t m_value;
};

class RespArray : public RespType {
public:
  RespArray() : RespType(ARRAY) {}

  RespArray(const RespArray &other) : RespArray() {
    for (const auto &val : other.m_value) {
      m_value.push_back(val->clone());
    }
  }

  std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespArray>(*this);
  }

  RespArray *add(std::unique_ptr<RespType> item) {
    m_value.push_back(std::move(item));
    return this;
  }

  const RespType *at(size_t idx) { return m_value.at(idx).get(); }

  std::vector<const RespType *> getValue() const {
    std::vector<const RespType *> ret;
    for (const auto &val : m_value) {
      ret.push_back(val.get());
    }
    return ret;
  }

  bool empty() const { return m_value.empty(); }

  std::vector<std::unique_ptr<RespType>> release() {
    return std::move(m_value);
  }

  virtual std::string serialize() override {
    std::string serialized_string("*");
    serialized_string += std::to_string(m_value.size()) + CRLF;
    for (const auto &element : m_value) {
      serialized_string += element->serialize();
    }
    return serialized_string;
  }

private:
  std::vector<std::unique_ptr<RespType>> m_value;
};

class RespBulkString : public RespType {
public:
  RespBulkString() : RespType(BULK_STRING), m_value(std::nullopt) {}

  RespBulkString(std::string val, bool lastCrlf = true)
      : RespType(BULK_STRING), m_value(std::move(val)), m_lastCrlf(lastCrlf) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespBulkString>(*this);
  }

  std::string getValue() const { return *m_value; }

  virtual std::string serialize() override {
    if (!m_value) {
      return std::string("$-1") + CRLF;
    }
    auto resp = std::string("$") + std::to_string(m_value->length()) + CRLF +
                getValue();
    if (m_lastCrlf) {
      resp += CRLF;
    }
    return resp;
  }

private:
  std::optional<std::string> m_value;
  bool m_lastCrlf;
};

class RespError : public RespType {
public:
  RespError(std::string val) : RespType(ERROR), m_value(std::move(val)) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespError>(*this);
  }

  std::string getValue() const { return m_value; }

  virtual std::string serialize() override {
    return std::string("-") + "ERR " + getValue() + CRLF;
  }

private:
  std::string m_value;
};

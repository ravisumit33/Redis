#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
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
  RespString(std::string val) : RespType(STRING), value(std::move(val)) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespString>(*this);
  }

  std::string getValue() const { return value; }

  virtual std::string serialize() override {
    return std::string("+") + getValue() + CRLF;
  }

private:
  std::string value;
};

class RespInt : public RespType {
public:
  RespInt(int64_t val) : RespType(INTEGER), value(val) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespInt>(*this);
  }

  int64_t getValue() const { return value; }

  virtual std::string serialize() override {
    return std::string(":") + std::to_string(getValue()) + CRLF;
  }

private:
  int64_t value;
};

class RespArray : public RespType {
public:
  RespArray() : RespType(ARRAY) {}

  RespArray(const RespArray &other) : RespArray() {
    for (const auto &val : other.value) {
      value.push_back(val->clone());
    }
  }

  std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespArray>(*this);
  }

  void add(std::unique_ptr<RespType> item) { value.push_back(std::move(item)); }

  const RespType *at(size_t idx) { return value.at(idx).get(); }

  std::vector<const RespType *> getValue() const {
    std::vector<const RespType *> ret;
    for (const auto &val : value) {
      ret.push_back(val.get());
    }
    return ret;
  }

  std::vector<std::unique_ptr<RespType>> release() { return std::move(value); }

  virtual std::string serialize() override {
    std::string serialized_string("*");
    if (value.empty()) {
      return serialized_string + "-1" + CRLF;
    }
    serialized_string += std::to_string(value.size()) + CRLF;
    for (const auto &element : value) {
      serialized_string += element->serialize();
    }
    return serialized_string;
  }

private:
  std::vector<std::unique_ptr<RespType>> value;
};

class RespBulkString : public RespType {
public:
  RespBulkString(std::string val)
      : RespType(BULK_STRING), value(std::move(val)) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespBulkString>(*this);
  }

  std::string getValue() const { return value; }

  virtual std::string serialize() override {
    if (value.empty()) {
      return std::string("$-1") + CRLF;
    }
    return std::string("$") + std::to_string(value.length()) + CRLF +
           getValue() + CRLF;
  }

private:
  std::string value;
};

class RespError : public RespType {
public:
  RespError(std::string val) : RespType(ERROR), value(std::move(val)) {}

  virtual std::unique_ptr<RespType> clone() override {
    return std::make_unique<RespError>(*this);
  }

  std::string getValue() const { return value; }

  virtual std::string serialize() override {
    return std::string("-") + getValue() + CRLF;
  }

private:
  std::string value;
};

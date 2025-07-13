#include "RespTypeParser.hpp"
#include "RespType.hpp"
#include <exception>
#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>
#include <string>

RespParserRegistrar<RespStringParser> RespStringParser::registrar('+');

std::unique_ptr<RespType> RespStringParser::parse(std::istream &in) {
  std::string line;
  std::getline(in, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("Invalid string");
  }
  return std::make_unique<RespString>(line);
}

RespParserRegistrar<RespIntParser> RespIntParser::registrar(':');

std::unique_ptr<RespType> RespIntParser::parse(std::istream &in) {
  std::string line;
  std::getline(in, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("Invalid integer");
  }

  int64_t val = 0;
  try {
    val = std::stoll(line);
  } catch (const std::exception &e) {
    throw std::runtime_error("Invalid integer");
  }

  return std::make_unique<RespInt>(val);
}

RespParserRegistrar<RespBulkStringParser> RespBulkStringParser::registrar('$');

std::unique_ptr<RespType> RespBulkStringParser::parse(std::istream &in) {
  std::string line;
  std::getline(in, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("Invalid bulk string");
  }

  size_t len = 0;
  try {
    len = std::stoul(line);
  } catch (const std::exception &e) {
    throw std::runtime_error("Invalid bulk string");
  }

  std::string val(len, '\0');
  in.read(val.data(), len);
  if (in.gcount() != len) {
    throw std::runtime_error("Invalid bulk string");
  }

  char cr, lf;
  in.get(cr);
  in.get(lf);
  if (cr != '\r' || lf != '\n') {
    throw std::runtime_error("Invalid bulk string");
  }

  return std::make_unique<RespBulkString>(std::move(val));
}

RespParserRegistrar<RespErrorParser> RespErrorParser::registrar('-');

std::unique_ptr<RespType> RespErrorParser::parse(std::istream &in) {
  std::string line;
  std::getline(in, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("Invalid error");
  }
  return std::make_unique<RespError>(std::move(line));
}

RespParserRegistrar<RespArrayParser> RespArrayParser::registrar('*');

std::unique_ptr<RespType> RespArrayParser::parse(std::istream &in) {
  std::string line;
  std::getline(in, line);

  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("Invalid array");
  }
  size_t len = 0;
  try {
    len = std::stoul(line);
  } catch (std::exception &e) {
    throw std::runtime_error("Invalide array");
  }

  if (len == 0) {
    throw std::runtime_error("Invalide array");
  }

  auto array = std::make_unique<RespArray>();
  for (int i = 0; i < len; ++i) {
    char prefix;
    in.get(prefix);
    if (!in) {
      throw std::runtime_error("Invalid array");
    }

    auto parser = RespParserRegistry::instance().get(prefix);
    if (!parser) {
      throw std::runtime_error("Invalid array");
    }
    auto element = parser->parse(in);
    array->add(std::move(element));
  }
  return array;
}

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
    throw std::runtime_error("RESP String: Missing \\r terminator in line: '" + line + "'");
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
    throw std::runtime_error("RESP Integer: Missing \\r terminator in line: '" + line + "'");
  }

  int64_t val = 0;
  try {
    val = std::stoll(line);
  } catch (const std::exception &e) {
    throw std::runtime_error("RESP Integer: Cannot parse '" + line + "' as integer: " + e.what());
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
    throw std::runtime_error("RESP BulkString: Missing \\r terminator in length line: '" + line + "'");
  }

  size_t len = 0;
  try {
    len = std::stoul(line);
  } catch (const std::exception &e) {
    throw std::runtime_error("RESP BulkString: Cannot parse length '" + line + "' as unsigned integer: " + e.what());
  }

  std::string val(len, '\0');
  in.read(val.data(), len);
  if (in.gcount() != static_cast<std::streamsize>(len)) {
    throw std::runtime_error("RESP BulkString: Expected " + std::to_string(len) + " bytes but got " + std::to_string(in.gcount()));
  }

  if (m_parseLastCrlf) {
    char cr, lf;
    in.get(cr);
    in.get(lf);
    if (cr != '\r' || lf != '\n') {
      throw std::runtime_error("RESP BulkString: Missing \\r\\n terminator after data (got '" + std::string(1, cr) + std::string(1, lf) + "')");
    }
  }

  return std::make_unique<RespBulkString>(std::move(val), m_parseLastCrlf);
}

RespParserRegistrar<RespErrorParser> RespErrorParser::registrar('-');

std::unique_ptr<RespType> RespErrorParser::parse(std::istream &in) {
  std::string line;
  std::getline(in, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("RESP Error: Missing \\r terminator in line: '" + line + "'");
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
    throw std::runtime_error("RESP Array: Missing \\r terminator in length line: '" + line + "'");
  }
  size_t len = 0;
  try {
    len = std::stoul(line);
  } catch (std::exception &e) {
    throw std::runtime_error("RESP Array: Cannot parse length '" + line + "' as unsigned integer: " + e.what());
  }

  if (len == 0) {
    throw std::runtime_error("RESP Array: Empty arrays not supported (length=0)");
  }

  auto array = std::make_unique<RespArray>();
  for (size_t i = 0; i < len; ++i) {
    char prefix;
    in.get(prefix);
    if (!in) {
      throw std::runtime_error("RESP Array: Unexpected end of stream at element " + std::to_string(i) + "/" + std::to_string(len));
    }

    auto parser = RespParserRegistry::instance().get(prefix);
    if (!parser) {
      throw std::runtime_error("RESP Array: Unknown RESP type '" + std::string(1, prefix) + "' at element " + std::to_string(i) + "/" + std::to_string(len));
    }
    
    try {
      auto element = parser->parse(in);
      array->add(std::move(element));
    } catch (const std::exception &e) {
      throw std::runtime_error("RESP Array: Failed to parse element " + std::to_string(i) + "/" + std::to_string(len) + ": " + e.what());
    }
  }
  return array;
}

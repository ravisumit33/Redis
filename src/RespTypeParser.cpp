#include "RespTypeParser.hpp"
#include "RespType.hpp"
#include <exception>
#include <istream>
#include <stdexcept>
#include <string>

RespString parseRespString(std::istream &in_stream) {
  std::string line;
  std::getline(in_stream, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("RESP String: Missing \\r terminator in line: '" +
                             line + "'");
  }
  return RespString(std::move(line));
}

RespInt parseRespInt(std::istream &in_stream) {
  std::string line;
  std::getline(in_stream, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("RESP Integer: Missing \\r terminator in line: '" +
                             line + "'");
  }

  int64_t val = 0;
  try {
    val = std::stoll(line);
  } catch (const std::exception &e) {
    throw std::runtime_error("RESP Integer: Cannot parse '" + line +
                             "' as integer: " + e.what());
  }

  return RespInt(val);
}

RespBulkString parseRespBulkString(std::istream &in_stream,
                                   bool parseLastCrlf) {
  std::string line;
  std::getline(in_stream, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error(
        "RESP BulkString: Missing \\r terminator in length line: '" + line +
        "'");
  }

  size_t len = 0;
  try {
    len = std::stoul(line);
  } catch (const std::exception &e) {
    throw std::runtime_error("RESP BulkString: Cannot parse length '" + line +
                             "' as unsigned integer: " + e.what());
  }

  std::string val(len, '\0');
  in_stream.read(val.data(), static_cast<std::streamsize>(len));
  if (in_stream.gcount() != static_cast<std::streamsize>(len)) {
    throw std::runtime_error("RESP BulkString: Expected " +
                             std::to_string(len) + " bytes but got " +
                             std::to_string(in_stream.gcount()));
  }

  if (parseLastCrlf) {
    char cr_char{};
    char lf_char{};
    in_stream.get(cr_char);
    in_stream.get(lf_char);
    if (cr_char != '\r' || lf_char != '\n') {
      throw std::runtime_error(
          "RESP BulkString: Missing \\r\\n terminator after data (got '" +
          std::string(1, cr_char) + std::string(1, lf_char) + "')");
    }
  }

  return RespBulkString(std::move(val), parseLastCrlf);
}

RespError parseRespError(std::istream &in_stream) {
  std::string line;
  std::getline(in_stream, line);
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error("RESP Error: Missing \\r terminator in line: '" +
                             line + "'");
  }
  return RespError(std::move(line));
}

// NOLINTNEXTLINE(misc-no-recursion)
RespArray parseRespArray(std::istream &in_stream) {
  std::string line;
  std::getline(in_stream, line);

  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  } else {
    throw std::runtime_error(
        "RESP Array: Missing \\r terminator in length line: '" + line + "'");
  }
  size_t len = 0;
  try {
    len = std::stoul(line);
  } catch (std::exception &e) {
    throw std::runtime_error("RESP Array: Cannot parse length '" + line +
                             "' as unsigned integer: " + e.what());
  }

  if (len == 0) {
    throw std::runtime_error(
        "RESP Array: Empty arrays not supported (length=0)");
  }

  RespArray array;
  for (size_t i = 0; i < len; ++i) {
    try {
      auto element = parseRespValue(in_stream);
      array.add(std::move(element));
    } catch (const std::exception &e) {
      throw std::runtime_error("RESP Array: Failed to parse element " +
                               std::to_string(i) + "/" + std::to_string(len) +
                               ": " + e.what());
    }
  }
  return array;
}

// NOLINTNEXTLINE(misc-no-recursion)
RespValue parseRespValue(std::istream &in_stream) {
  char prefix{};
  in_stream.get(prefix);
  if (!in_stream) {
    throw std::runtime_error(
        "RESP Value: Unexpected end of stream (no prefix char)");
  }

  switch (prefix) {
  case '+':
    return parseRespString(in_stream);
  case ':':
    return parseRespInt(in_stream);
  case '$':
    return parseRespBulkString(in_stream);
  case '-':
    return parseRespError(in_stream);
  case '*':
    return parseRespArray(in_stream);
  default:
    throw std::runtime_error("RESP Value: Unknown RESP type prefix '" +
                             std::string(1, prefix) + "'");
  }
}

#include "utils/genericUtils.hpp"
#include "RespType.hpp"
#include "redis_store/values/StreamValue.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

uint8_t hexCharToInt(char hex_char) {
  if ('0' <= hex_char && hex_char <= '9') {
    return hex_char - '0';
  }
  constexpr uint8_t decimal = 10;
  if ('a' <= hex_char && hex_char <= 'f') {
    return hex_char - 'a' + decimal;
  }
  if ('A' <= hex_char && hex_char <= 'F') {
    return hex_char - 'A' + decimal;
  }
  throw std::invalid_argument("Invalid hex character");
}

std::string hexToBinary(const std::string &hex) {
  if (hex.size() % 2 != 0) {
    throw std::invalid_argument("Hex string must have even length");
  }

  std::string result;
  result.reserve(hex.size() / 2);
  for (size_t i = 0; i < hex.size(); i += 2) {
    const uint8_t high = hexCharToInt(hex[i]);
    const uint8_t low = hexCharToInt(hex[i + 1]);
    result.push_back(static_cast<char>((high << 4) | low));
  }
  return result;
}

std::string generateRandomHexId() {
  std::random_device rnd;
  std::mt19937 gen(rnd());
  constexpr uint8_t byte_limit = 255;
  std::uniform_int_distribution<uint8_t> dis(0, byte_limit);

  std::ostringstream oss;
  constexpr uint8_t id_length = 20;
  for (size_t i = 0; i < id_length; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(dis(gen));
  }
  return oss.str();
}

std::array<uint64_t, 2> parseStreamEntryId(const std::string &entry_id) {
  const std::size_t hyphen_pos = entry_id.find('-');
  const uint64_t timestamp = std::stoull(entry_id.substr(0, hyphen_pos));
  uint64_t sequence = 0;
  if (hyphen_pos != std::string::npos) {
    sequence = std::stoull(entry_id.substr(hyphen_pos + 1));
  }
  return {timestamp, sequence};
}

std::array<std::optional<uint64_t>, 2>
parsePartialStreamEntryId(const std::string &entry_id) {
  using EntryIdPart = std::variant<uint64_t, std::string>;
  const std::size_t hyphen_pos = entry_id.find('-');
  if (hyphen_pos == std::string::npos) {
    throw std::runtime_error("Invalid entry_id for stream");
  }

  auto parsePart = [](const std::string &part) -> EntryIdPart {
    try {
      return std::stoull(part);
    } catch (...) {
      return part;
    }
  };

  auto part1 = parsePart(entry_id.substr(0, hyphen_pos));
  auto part2 = parsePart(entry_id.substr(hyphen_pos + 1));
  std::optional<uint64_t> timestamp;
  std::optional<uint64_t> sequence;

  auto extractPart = [](const EntryIdPart &part,
                        std::optional<uint64_t> &target, const char *which) {
    if (const auto *id_part = std::get_if<uint64_t>(&part)) {
      target = *id_part;
    } else if (const auto *str = std::get_if<std::string>(&part)) {
      if (*str != "*") {
        throw std::runtime_error(
            std::string("Invalid entry_id component in XADD: ") + which);
      }
    } else {
      throw std::runtime_error("Unhandled variant type in entry_id part");
    }
  };

  extractPart(part1, timestamp, "timestamp");
  extractPart(part2, sequence, "sequence");

  return {timestamp, sequence};
}

RespValue serializeStreamEntries(
    std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
        entries) {

  RespArray resp_array;
  for (auto &[entry_id, entry] : entries) {
    RespArray entry_array;
    auto entry_id_str =
        std::to_string(entry_id.at(0)) + "-" + std::to_string(entry_id.at(1));
    entry_array.add(RespBulkString(std::move(entry_id_str)));
    RespArray values_array;
    for (auto &[key, value] : entry) {
      values_array.add(RespBulkString(key));
      values_array.add(RespBulkString(std::move(value)));
    }
    entry_array.add(std::move(values_array));
    resp_array.add(std::move(entry_array));
  }

  return resp_array;
}

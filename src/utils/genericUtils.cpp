#include "utils/genericUtils.hpp"
#include "AppContext.hpp"
#include "RespTypeParser.hpp"
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <istream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
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
    uint8_t high = hexCharToInt(hex[i]);
    uint8_t low = hexCharToInt(hex[i + 1]);
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

std::string readFromSocket(unsigned socket_fd) {
  constexpr size_t buffer_size = 1024;
  std::array<char, buffer_size> buffer{};
  ssize_t bytes_received =
      recv(static_cast<int>(socket_fd), buffer.data(), buffer.size() - 1, 0);
  if (bytes_received <= 0) {
    return "";
  }
  buffer.at(static_cast<size_t>(bytes_received)) = '\0';
  return {buffer.data(), static_cast<size_t>(bytes_received)};
}

void writeToSocket(unsigned socket_fd, const std::string &data) {
  std::cout << "Sent: " << data << " to [fd=" << socket_fd << "]" << '\n';
  send(static_cast<int>(socket_fd), data.c_str(), data.length(), 0);
}

std::pair<std::unique_ptr<Command>, std::vector<RespValue>>
parseCommand(std::istream &in_stream, AppContext &context) {
  char type{};
  in_stream.get(type);
  if (!in_stream) {
    throw std::runtime_error(
        "Command parsing: No RESP type found (empty stream)");
  }

  if (type != '*') {
    throw std::runtime_error(
        "Command parsing: Expected array type '*' but got '" +
        std::string(1, type) + "' (commands must be arrays)");
  }

  RespArray cmd_arr = parseRespArray(in_stream);
  auto command_args = cmd_arr.release();
  if (command_args.empty()) {
    throw std::runtime_error("Command parsing: Empty command array received");
  }

  auto &command_name_val = command_args.at(0);
  if (command_name_val.getType() != RespType::BULK_STRING) {
    throw std::runtime_error(
        "Command parsing: Command name must be bulk string, got type " +
        std::to_string(static_cast<int>(command_name_val.getType())));
  }

  auto cmd_name = getStringValue(command_name_val);

  auto command = context.getCommandRegistry().get(cmd_name);
  if (!command) {
    throw std::runtime_error("Command parsing: Unsupported command '" +
                             cmd_name + "'");
  }
  command_args.erase(command_args.begin());

  return {std::move(command), std::move(command_args)};
}

std::array<uint64_t, 2> parseStreamEntryId(const std::string &entry_id) {
  std::size_t hyphen_pos = entry_id.find('-');
  uint64_t timestamp = std::stoull(entry_id.substr(0, hyphen_pos));
  uint64_t sequence = 0;
  if (hyphen_pos != std::string::npos) {
    sequence = std::stoull(entry_id.substr(hyphen_pos + 1));
  }
  return {timestamp, sequence};
}

std::array<std::optional<uint64_t>, 2>
parsePartialStreamEntryId(const std::string &entry_id) {
  using EntryIdPart = std::variant<uint64_t, std::string>;
  std::size_t hyphen_pos = entry_id.find('-');
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

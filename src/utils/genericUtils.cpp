#include "utils/genericUtils.hpp"
#include "RespTypeParser.hpp"
#include <array>
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

uint8_t hexCharToInt(char c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  if ('A' <= c && c <= 'F')
    return c - 'A' + 10;
  throw std::invalid_argument("Invalid hex character");
}

std::string hexToBinary(const std::string &hex) {
  if (hex.size() % 2 != 0)
    throw std::invalid_argument("Hex string must have even length");

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
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);

  std::ostringstream oss;
  for (size_t i = 0; i < 20; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(dis(gen));
  }
  return oss.str();
}

std::string readFromSocket(unsigned socket_fd) {
  char buffer[1024];
  ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0) {
    return "";
  }
  buffer[bytes_received] = '\0';
  return std::string(buffer, bytes_received);
}

void writeToSocket(unsigned socket_fd, const std::string &data) {
  std::cout << "Sent: " << data << " to [fd=" << socket_fd << "]" << std::endl;
  send(socket_fd, data.c_str(), data.length(), 0);
}

std::pair<std::unique_ptr<Command>, std::vector<std::unique_ptr<RespType>>>
parseCommand(std::istream &in) {
  char type;
  in.get(type);
  if (!in) {
    throw std::runtime_error(
        "Command parsing: No RESP type found (empty stream)");
  }

  if (type != '*') {
    throw std::runtime_error(
        "Command parsing: Expected array type '*' but got '" +
        std::string(1, type) + "' (commands must be arrays)");
  }

  auto parser = RespParserRegistry::instance().get(type);
  if (!parser) {
    throw std::logic_error(
        "Command parsing: No parser found for array type '*' (internal error)");
  }

  auto parsed_command = parser->parse(in);
  RespArray &cmd_arr = static_cast<RespArray &>(*parsed_command);

  auto command_args = cmd_arr.release();
  if (command_args.empty()) {
    throw std::runtime_error("Command parsing: Empty command array received");
  }

  auto command_name_ptr = command_args.at(0).get();
  if (command_name_ptr->getType() != RespType::BULK_STRING) {
    throw std::runtime_error(
        "Command parsing: Command name must be bulk string, got type " +
        std::to_string(command_name_ptr->getType()));
  }

  auto cmd_name =
      static_cast<const RespBulkString &>(*command_name_ptr).getValue();

  auto command = CommandRegistry::instance().get(cmd_name);
  if (!command) {
    throw std::runtime_error("Command parsing: Unsupported command '" +
                             cmd_name + "'");
  }
  command_args.erase(command_args.begin());

  return {std::move(command), std::move(command_args)};
}

std::array<uint64_t, 2> parseStreamEntryId(const std::string &s) {
  std::size_t hyphen_pos = s.find("-");
  uint64_t timestamp = std::stoull(s.substr(0, hyphen_pos));
  uint64_t sequence = 0;
  if (hyphen_pos != std::string::npos) {
    sequence = std::stoull(s.substr(hyphen_pos + 1));
  }
  return {timestamp, sequence};
}

std::array<std::optional<uint64_t>, 2>
parsePartialStreamEntryId(const std::string &entry_id) {
  using EntryIdPart = std::variant<uint64_t, std::string>;
  std::size_t hyphen_pos = entry_id.find("-");
  if (hyphen_pos == std::string::npos) {
    throw std::runtime_error("Invalid entry_id for stream");
  }

  auto parsePart = [](const std::string &s) -> EntryIdPart {
    try {
      return std::stoull(s);
    } catch (...) {
      return s;
    }
  };

  auto part1 = parsePart(entry_id.substr(0, hyphen_pos));
  auto part2 = parsePart(entry_id.substr(hyphen_pos + 1));
  std::optional<uint64_t> timestamp, sequence;

  auto extractPart = [](const EntryIdPart &part,
                        std::optional<uint64_t> &target, const char *which) {
    if (auto p = std::get_if<uint64_t>(&part)) {
      target = *p;
    } else if (auto str = std::get_if<std::string>(&part)) {
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

std::unique_ptr<RespArray> serializeStreamEntries(
    std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
        entries) {

  auto resp_array = std::make_unique<RespArray>();
  for (auto &[entry_id, entry] : entries) {
    auto entry_array = std::make_unique<RespArray>();
    auto entry_id_str =
        std::to_string(entry_id.at(0)) + "-" + std::to_string(entry_id.at(1));
    entry_array->add(std::make_unique<RespBulkString>(std::move(entry_id_str)));
    auto values_array = std::make_unique<RespArray>();
    for (auto &[key, value] : entry) {
      values_array->add(std::make_unique<RespBulkString>(std::move(key)));
      values_array->add(std::make_unique<RespBulkString>(std::move(value)));
    }
    entry_array->add(std::move(values_array));
    resp_array->add(std::move(entry_array));
  }

  return resp_array;
}

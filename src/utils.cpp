#include "utils.hpp"
#include "Command.hpp"
#include "RespTypeParser.hpp"
#include <iomanip>
#include <iostream>
#include <istream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>
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

std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
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

  return {command, std::move(command_args)};
}

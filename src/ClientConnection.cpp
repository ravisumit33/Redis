#include "ClientConnection.hpp"
#include "AppConfig.hpp"
#include "Command.hpp"
#include "RespType.hpp"
#include "RespTypeParser.hpp"
#include "unistd.h"
#include <exception>
#include <iostream>
#include <istream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>

ClientConnection::ClientConnection(const unsigned socket_fd,
                                   const AppConfig &config)
    : m_socket_fd(socket_fd), m_config(config) {}

ClientConnection::~ClientConnection() { close(m_socket_fd); }

std::string ClientConnection::readFromSocket() {
  char buffer[1024];
  ssize_t bytes_received = recv(m_socket_fd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0) {
    return "";
  }
  buffer[bytes_received] = '\0';
  return std::string(buffer);
}

void ClientConnection::writeToSocket(const std::string &data) {
  std::cout << "Sent: " << data << std::endl;
  send(m_socket_fd, data.c_str(), data.length(), 0);
}

void ClientConnection::handleClient() {
  try {
    while (true) {
      std::string data = readFromSocket();
      if (data.empty()) {
        std::cout << "Client disconnected" << std::endl;
        break;
      }

      std::cout << "Received: " << data << std::endl;

      std::stringstream ss(data);
      try {
        auto [command, args] = parseCommand(ss);
        auto response = command->execute(std::move(args), m_config);
        writeToSocket(response->serialize());
      } catch (const std::exception &ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
        RespError errResponse("Unknown command");
        writeToSocket(errResponse.serialize());
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Error handling client: " << exp.what() << std::endl;
  }
}

std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
ClientConnection::parseCommand(std::istream &in) {
  char type;
  in.get(type);
  if (!in) {
    throw std::runtime_error("No command RESP type");
  }

  if (type != '*') {
    throw std::runtime_error("Unexpected command RESP type");
  }

  auto parser = RespParserRegistry::instance().get(type);
  if (!parser) {
    throw std::runtime_error("Malformed command");
  }
  auto parsed_command = parser->parse(in);
  RespArray &cmd_arr = static_cast<RespArray &>(*parsed_command);

  auto command_args = cmd_arr.release();
  auto command_name_ptr = command_args[0].get();
  if (command_name_ptr->getType() != RespType::BULK_STRING) {
    throw std::runtime_error("Unexpected command name RESP type");
  }

  auto cmd_name =
      static_cast<const RespBulkString &>(*command_name_ptr).getValue();
  auto command = CommandRegistry::instance().get(cmd_name);
  if (!command) {
    throw std::runtime_error("Command not supported");
  }
  command_args.erase(command_args.begin());

  return {command, std::move(command_args)};
}

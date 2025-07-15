#include "Connection.hpp"
#include "AppConfig.hpp"
#include "Command.hpp"
#include "RespType.hpp"
#include "RespTypeParser.hpp"
#include "unistd.h"
#include <exception>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>

Connection::Connection(const unsigned socket_fd, const AppConfig &config)
    : m_socket_fd(socket_fd), m_config(config) {}

Connection::~Connection() { close(m_socket_fd); }

std::string Connection::readFromSocket() const {
  char buffer[1024];
  ssize_t bytes_received = recv(m_socket_fd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0) {
    return "";
  }
  buffer[bytes_received] = '\0';
  return std::string(buffer);
}

void Connection::writeToSocket(const std::string &data) const {
  std::cout << "Sent: " << data << std::endl;
  send(m_socket_fd, data.c_str(), data.length(), 0);
}

std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
Connection::parseCommand(std::istream &in) const {
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

void ClientConnection::handleConnection() {
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
        auto response = command->execute(std::move(args), getConfig());
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

void ServerConnection::validateResponse(const std::string &expectedResponse) {
  auto servResponse = readFromSocket();
  std::istringstream in(servResponse);
  char type;
  in.get(type);
  if (!in) {
    throw std::runtime_error("No response RESP type");
  }

  if (type != '+') {
    throw std::runtime_error("Unexpected response RESP type");
  }

  auto parser = RespParserRegistry::instance().get(type);
  if (!parser) {
    throw std::runtime_error("Malformed response");
  }
  auto parsed_response = parser->parse(in);
  RespString &response = static_cast<RespString &>(*parsed_response);
  if (const auto &resp = response.getValue(); resp != expectedResponse) {
    std::cerr << "Expected " << expectedResponse << " but got " << resp
              << std::endl;
    throw std::runtime_error("Unexpected response");
  }
}

void ServerConnection::handShake() {
  auto array = std::make_unique<RespArray>();
  auto init_msg = std::make_unique<RespBulkString>("PING");
  array->add(std::move(init_msg));
  writeToSocket(array->serialize());
  validateResponse("PONG");
}

void ServerConnection::configureRepl() {
  auto array = std::make_unique<RespArray>();
  auto replconf = std::make_unique<RespBulkString>("REPLCONF");
  auto lp = std::make_unique<RespBulkString>("listening-port");
  auto port =
      std::make_unique<RespBulkString>(std::to_string(getConfig().getPort()));
  array->add(std::move(replconf))->add(std::move(lp))->add(std::move(port));
  writeToSocket(array->serialize());
  validateResponse("OK");

  array = std::make_unique<RespArray>();
  replconf = std::make_unique<RespBulkString>("REPLCONF");
  auto capa = std::make_unique<RespBulkString>("capa");
  auto psync = std::make_unique<RespBulkString>("psync2");
  array->add(std::move(replconf))->add(std::move(capa))->add(std::move(psync));
  writeToSocket(array->serialize());
  validateResponse("OK");
}

void ServerConnection::handleConnection() {
  std::cout << "Handling server connection" << std::endl;
  handShake();
  std::cout << "Handshake done" << std::endl;
  configureRepl();
  std::cout << "Repl configuration done" << std::endl;
}

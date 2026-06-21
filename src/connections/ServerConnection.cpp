#include "connections/ServerConnection.hpp"
#include "AppContext.hpp"
#include "CommandExecutor.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "RespTypeParser.hpp"
#include "commands/ReplConfCommand.hpp"
#include "utils/SocketUtils.hpp"
#include <cstddef>
#include <exception>
#include <functional>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

void ServerConnection::sendCommand(std::vector<RespValue> args) const {
  RespArray array;
  for (auto &resp_val : args) {
    array.add(std::move(resp_val));
  }
  SocketUtils::writeToSocket(getSocketFd(), array.serialize());
}

void ServerConnection::validateResponse(auto validate) {
  auto servResponse = SocketUtils::readFromSocket(getSocketFd());
  std::cout << "Received: " << servResponse << " from master: " << getSocketFd()
            << '\n';
  std::istringstream in_stream(servResponse);
  char type{};
  in_stream.get(type);
  if (!in_stream) {
    throw std::runtime_error("No response RESP type");
  }

  if (type != '+') {
    throw std::runtime_error(
        "Replication handshake: Expected simple string '+' response, got '" +
        std::string(1, type) + "'");
  }

  const RespString response = parseRespString(in_stream);
  validate(response.getValue(), in_stream);
}

void ServerConnection::configureRepl() {
  std::vector<RespValue> cmd;
  cmd.emplace_back(RespBulkString("REPLCONF"));
  cmd.emplace_back(RespBulkString("listening-port"));
  cmd.emplace_back(
      RespBulkString(std::to_string(getContext().getConfig().getPort())));
  sendCommand(std::move(cmd));
  validateResponse(
      [](const std::string &response, std::istringstream & /*in_stream*/) {
        if (response != "OK") {
          std::cerr << "REPLCONF listening-port: Expected 'OK' but got '"
                    << response << "'" << '\n';
          throw std::runtime_error("REPLCONF listening-port failed: got '" +
                                   response + "' instead of 'OK'");
        }
      });

  cmd.clear();
  cmd.emplace_back(RespBulkString("REPLCONF"));
  cmd.emplace_back(RespBulkString("capa"));
  cmd.emplace_back(RespBulkString("psync2"));
  sendCommand(std::move(cmd));
  validateResponse(
      [](const std::string &response, std::istringstream & /*in*/) {
        if (response != "OK") {
          std::cerr << "REPLCONF capa: Expected 'OK' but got '" << response
                    << "'" << '\n';
          throw std::runtime_error("REPLCONF capa failed: got '" + response +
                                   "' instead of 'OK'");
        }
      });
}

void ServerConnection::processBacklogCommands(
    std::istringstream &input_stream) const {
  const std::streampos pos = input_stream.tellg();
  const std::size_t total_bytes_after_handshake =
      (pos != -1) ? (input_stream.str().size() - static_cast<std::size_t>(pos))
                  : 0;
  std::size_t bytes_recorded = 0;
  auto ctx = makeExecContext();

  while (input_stream.peek() != std::char_traits<char>::eof()) {
    auto [command, args] = parseCommand(input_stream, getContext());
    auto responses = executeCommand(command, args, ctx);
    std::string resp;
    for (const auto &response : responses) {
      resp += response.serialize();
    }
    if (std::holds_alternative<ReplConfCommand>(command)) {
      SocketUtils::writeToSocket(getSocketFd(), resp);
    }
    const std::streampos cur_pos = input_stream.tellg();
    const std::size_t bytes_remaining =
        (cur_pos != -1)
            ? (input_stream.str().size() - static_cast<std::size_t>(cur_pos))
            : 0;
    const std::size_t bytes_read =
        total_bytes_after_handshake - bytes_remaining;
    const std::size_t new_bytes_read = bytes_read - bytes_recorded;
    auto &slave_state = getContext().getReplicationManager().slave();
    slave_state.addBytesProcessed(new_bytes_read);
    bytes_recorded += new_bytes_read;
  }
  std::cout << "Backlog commands executed" << '\n';
}

void ServerConnection::configurePsync() {
  std::vector<RespValue> cmd;
  cmd.emplace_back(RespBulkString("PSYNC"));
  cmd.emplace_back(RespBulkString("?"));
  cmd.emplace_back(RespBulkString("-1"));
  sendCommand(std::move(cmd));
  validateResponse([this](const std::string &response,
                          std::istringstream &in_stream) {
    std::istringstream stream(response);
    stream.exceptions(std::ios::badbit);
    std::string psync_cmd;
    std::string repl_id;
    unsigned repl_offset = 0;
    stream >> psync_cmd >> repl_id >> repl_offset;
    if (psync_cmd != "FULLRESYNC") {
      std::cerr << "PSYNC: Expected 'FULLRESYNC' but got '" << response << "'"
                << '\n';
      throw std::runtime_error("PSYNC failed: expected 'FULLRESYNC' but got '" +
                               response + "'");
    }
    std::cout << "FULLRESYNC recieved" << '\n';

    auto extractRDB = [this](std::istream &ins) -> bool {
      char type = 0;
      ins.get(type);
      if (!ins || type != '$') {
        return false;
      }
      const RespBulkString rdb_bulk_string = parseRespBulkString(ins, false);
      std::cout << "Received RDB file from master [fd=" << getSocketFd() << "]"
                << '\n';
      return true;
    };

    std::reference_wrapper<std::istringstream> input_stream_ref = in_stream;
    std::optional<std::istringstream> next_server_response;

    if (in_stream.peek() != std::char_traits<char>::eof()) {
      if (!extractRDB(in_stream)) {
        throw std::runtime_error("Unexpected RDB file");
      }
    } else {
      const std::string servResponse =
          SocketUtils::readFromSocket(getSocketFd());
      std::cout << "Received: " << servResponse
                << " from master [fd=" << getSocketFd() << "]" << '\n';
      next_server_response.emplace(servResponse);
      if (!extractRDB(*next_server_response)) {
        throw std::runtime_error("Unexpected RDB file");
      }
      input_stream_ref = *next_server_response;
    }

    processBacklogCommands(input_stream_ref.get());
  });
}

void ServerConnection::handShake() {
  std::vector<RespValue> cmd;
  cmd.emplace_back(RespBulkString("PING"));
  sendCommand(std::move(cmd));
  validateResponse(
      [](const std::string &response, std::istringstream & /*in_stream*/) {
        if (response != "PONG") {
          std::cerr << "PING handshake: Expected 'PONG' but got '" << response
                    << "'" << '\n';
          throw std::runtime_error("PING handshake failed: got '" + response +
                                   "' instead of 'PONG'");
        }
      });
  std::cout << "PING complete" << '\n';

  configureRepl();
  std::cout << "Repl configured" << '\n';

  configurePsync();
  std::cout << "Psync configured" << '\n';
}

void ServerConnection::handleConnection() {
  handShake();
  m_ready_callback();
  std::cout << "Handshake done" << '\n';

  try {
    auto ctx = makeExecContext();
    while (true) {
      const std::string data = SocketUtils::readFromSocket(getSocketFd());
      if (data.empty()) {
        std::cout << "Server disconnected at: " << getSocketFd() << '\n';
        break;
      }

      std::cout << "Received: " << data << " from master: " << getSocketFd()
                << '\n';

      std::stringstream socket_data(data);
      const std::size_t total_bytes_received = data.size();
      std::size_t bytes_recorded = 0;
      while (socket_data.peek() != std::char_traits<char>::eof()) {
        auto [command, args] = parseCommand(socket_data, getContext());
        auto responses = executeCommand(command, args, ctx);
        std::string serialized_response;
        for (const auto &response : responses) {
          serialized_response += response.serialize();
        }
        if (std::holds_alternative<ReplConfCommand>(command)) {
          SocketUtils::writeToSocket(getSocketFd(), serialized_response);
        }
        const std::streampos pos = socket_data.tellg();
        const std::size_t bytes_remaining =
            (pos != -1)
                ? (socket_data.str().size() - static_cast<std::size_t>(pos))
                : 0;
        const std::size_t bytes_read = total_bytes_received - bytes_remaining;
        const std::size_t new_bytes_read = bytes_read - bytes_recorded;
        auto &slave_state = getContext().getReplicationManager().slave();
        slave_state.addBytesProcessed(new_bytes_read);
        bytes_recorded += new_bytes_read;
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Server connection error [fd=" << getSocketFd()
              << "]: " << exp.what() << '\n';
  }
}

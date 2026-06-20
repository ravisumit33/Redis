#include "RedisServer.hpp"
#include "AppContext.hpp"
#include "RdbParser.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "utils/SocketUtils.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

RedisServer::RedisServer(AppConfig config) : m_context(std::move(config)) {
  m_context.initialize();
}

void RedisServer::start() {

  if (auto smetadata = m_context.getConfig().getSlaveConfig(); smetadata) {
    auto [master_host, master_port] = *smetadata;
    std::cout << "Attaching slave to master: " << master_host << " "
              << master_port << '\n';

    m_master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_master_fd < 0) {
      throw std::runtime_error("Failed to create master socket: " +
                               std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(m_master_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) < 0) {
      close(m_master_fd);
      throw std::runtime_error("setsockopt failed while connecting to master");
    }

    struct sockaddr_in master_addr{};
    master_addr.sin_family = AF_INET;
    inet_pton(AF_INET,
              master_host == "localhost" ? "127.0.0.1" : master_host.c_str(),
              &master_addr.sin_addr);
    master_addr.sin_port = htons(master_port);

    if (connect(m_master_fd, SocketUtils::toSockAddr(&master_addr),
                sizeof(master_addr)) != 0) {
      throw std::runtime_error("Failed to connect to master");
    }

    std::cout << "Connected to master server [fd=" << m_master_fd
              << ", master=" << master_host << ":" << master_port << "]"
              << '\n';

    auto self = shared_from_this();
    auto ready_callback = [self]() { self->m_init_done = true; };
    // `self` keeps RedisServer (and m_context) alive for the thread's lifetime.
    std::thread master_thread([self, callback = ready_callback]() {
      ServerConnection master_con(self->m_master_fd, self->m_context, callback);
      master_con.handleConnection();
    });
    master_thread.detach();
  } else {
    const auto &mmetadata = m_context.getConfig().getMasterConfig();
    auto rdb_path =
        std::filesystem::path(mmetadata->dir) / mmetadata->dbfilename;
    std::ifstream rdb_fstream(rdb_path);
    if (rdb_fstream) {
      RdbParserRegistry rdb_registry;
      registerRdbParsers(rdb_registry);
      RdbParseState state;
      RdbHeaderParser::parse(rdb_fstream, rdb_registry,
                             m_context.getRedisStore(), state);
      while (rdb_fstream.peek() != std::char_traits<char>::eof()) {
        const char opc = static_cast<char>(rdb_fstream.get());
        if (!rdb_fstream) {
          throw std::runtime_error("Failure reading rdb file");
        }
        auto rdb_opcode = static_cast<RdbOpcode>(static_cast<uint8_t>(opc));
        auto parser = rdb_registry.get(rdb_opcode);
        if (!parser) {
          throw std::runtime_error(
              "Invalid opcode: " +
              std::to_string(static_cast<uint8_t>(rdb_opcode)) +
              " found during RDB parsing");
        }
        parseRdb(*parser, rdb_fstream, rdb_registry, m_context.getRedisStore(),
                 state);
      }
    }
    m_init_done = true;
  }

  setupSocket();
  m_isRunning = true;
  acceptConnections();
}

RedisServer::~RedisServer() { stop(); }

void RedisServer::stop() {
  if (m_isRunning) {
    assert(m_server_fd != -1);
    close(m_server_fd);
    m_server_fd = -1;
    m_isRunning = false;
  }
  if (m_master_fd != -1) {
    close(m_master_fd);
    m_master_fd = -1;
  }
  std::cout << "Redis server stopped" << '\n';
}

void RedisServer::setupSocket() {
  m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_server_fd < 0) {
    throw std::runtime_error("Failed to create server socket: " +
                             std::string(strerror(errno)));
  }

  // Set SO_REUSEADDR to avoid "Address already in use" errors
  int reuse = 1;
  if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    close(m_server_fd);
    throw std::runtime_error("setsockopt failed");
  }

  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(m_context.getConfig().getPort());

  if (bind(m_server_fd, SocketUtils::toSockAddr(&server_addr),
           sizeof(server_addr)) != 0) {
    close(m_server_fd);
    throw std::runtime_error("Failed to bind to port " +
                             std::to_string(m_context.getConfig().getPort()));
  }

  constexpr int connection_backlog = 5;
  if (listen(m_server_fd, connection_backlog) != 0) {
    close(m_server_fd);
    throw std::runtime_error("listen failed on port " +
                             std::to_string(m_context.getConfig().getPort()));
  }
  std::cout << "Server socket listening on port "
            << m_context.getConfig().getPort() << " [fd=" << m_server_fd << "]"
            << '\n';
}

void RedisServer::acceptConnections() {
  struct sockaddr_in client_addr{};
  socklen_t client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for clients to connect..." << '\n';

  while (m_isRunning) {
    const int client_fd = accept(
        m_server_fd, SocketUtils::toSockAddr(&client_addr), &client_addr_len);

    if (client_fd < 0) {
      if (m_isRunning) {
        std::cerr << "Failed to accept client connection" << '\n';
      }
      continue;
    }

    std::cout << "Client connected [fd=" << client_fd << "]" << '\n';

    try {

      constexpr uint8_t WAIT_SLEEP_DURATION = 10;
      while (!m_init_done) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(WAIT_SLEEP_DURATION));
      }

      auto self = shared_from_this();
      // `self` keeps RedisServer (and m_context) alive for the thread's
      // lifetime.
      std::thread client_thread([self, client_fd]() {
        try {
          ClientConnection client_con(client_fd, self->m_context);
          client_con.handleConnection();
        } catch (const std::exception &e) {
          std::cerr << "Error in client thread [fd=" << client_fd
                    << "]: " << e.what() << '\n';
        } catch (...) {
          std::cerr << "Unknown error in client thread [fd=" << client_fd << "]"
                    << '\n';
        }
      });
      client_thread.detach();
    } catch (const std::exception &e) {
      std::cerr << "Error starting client thread [fd=" << client_fd
                << "]: " << e.what() << '\n';
      close(client_fd);
    }
  }
}

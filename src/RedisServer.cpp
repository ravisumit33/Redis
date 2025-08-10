#include "RedisServer.hpp"
#include "AppConfig.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "RdbParser.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <chrono>
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

RedisServer::RedisServer(const AppConfig &config) : m_config(config) {}

void RedisServer::start() {

  if (auto smetadata = m_config.getSlaveConfig(); smetadata) {
    auto [master_host, master_port] = *smetadata;
    std::cout << "Attaching slave to master: " << master_host << " "
              << master_port << std::endl;

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

    struct sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    inet_pton(AF_INET,
              master_host == "localhost" ? "127.0.0.1" : master_host.c_str(),
              &master_addr.sin_addr);
    master_addr.sin_port = htons(master_port);

    if (connect(m_master_fd, (struct sockaddr *)&master_addr,
                sizeof(master_addr)) != 0) {
      throw std::runtime_error("Failed to connect to master");
    }

    std::cout << "Connected to master server [fd=" << m_master_fd
              << ", master=" << master_host << ":" << master_port << "]"
              << std::endl;

    auto self = shared_from_this();
    auto ready_callback = [self]() { self->m_init_done = true; };
    std::thread master_thread([self, callback = ready_callback]() {
      ServerConnection master_con(self->m_master_fd, self->m_config, callback);
      master_con.handleConnection();
    });
    master_thread.detach();
  } else {
    auto &mmetadata = m_config.getMasterConfig();
    auto rdb_path =
        std::filesystem::path(mmetadata->dir) / mmetadata->dbfilename;
    std::ifstream rdb_fstream(rdb_path);
    if (rdb_fstream) {
      RdbHeaderParser rdb_header_parser;
      rdb_header_parser.parse(rdb_fstream);
      while (rdb_fstream.peek() != std::char_traits<char>::eof()) {
        char opc = rdb_fstream.get();
        if (!rdb_fstream) {
          throw std::runtime_error("Failure reading rdb file");
        }
        RdbOpcode rdb_opcode =
            static_cast<RdbOpcode>(static_cast<uint8_t>(opc));
        auto parser = RdbParserRegistry::instance().get(rdb_opcode);
        if (!parser) {
          throw std::runtime_error(
              "Invalid opcode: " +
              std::to_string(static_cast<uint8_t>(rdb_opcode)) +
              " found during RDB parsing");
        }
        parser->parse(rdb_fstream);
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
  std::cout << "Redis server stopped" << std::endl;
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

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(m_config.getPort());

  if (bind(m_server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    close(m_server_fd);
    throw std::runtime_error("Failed to bind to port " +
                             std::to_string(m_config.getPort()));
  }

  int connection_backlog = 5;
  if (listen(m_server_fd, connection_backlog) != 0) {
    close(m_server_fd);
    throw std::runtime_error("listen failed on port " +
                             std::to_string(m_config.getPort()));
  }
  std::cout << "Server socket listening on port " << m_config.getPort()
            << " [fd=" << m_server_fd << "]" << std::endl;
}

void RedisServer::acceptConnections() {
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for clients to connect..." << std::endl;

  while (m_isRunning) {
    int client_fd = accept(m_server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);

    if (client_fd < 0) {
      if (m_isRunning) {
        std::cerr << "Failed to accept client connection" << std::endl;
      }
      continue;
    }

    std::cout << "Client connected [fd=" << client_fd << "]" << std::endl;

    try {

      while (!m_init_done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      auto self = shared_from_this();
      std::thread client_thread([self, client_fd]() {
        try {
          ClientConnection client_con(client_fd, self->m_config);
          client_con.handleConnection();
        } catch (const std::exception &e) {
          std::cerr << "Error in client thread [fd=" << client_fd
                    << "]: " << e.what() << std::endl;
        } catch (...) {
          std::cerr << "Unknown error in client thread [fd=" << client_fd << "]"
                    << std::endl;
        }
      });
      client_thread.detach();
    } catch (const std::exception &e) {
      std::cerr << "Error starting client thread [fd=" << client_fd
                << "]: " << e.what() << std::endl;
      close(client_fd);
    }
  }
}

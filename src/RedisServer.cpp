#include "RedisServer.hpp"
#include "AppConfig.hpp"
#include "Connection.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

RedisServer::RedisServer(const AppConfig &config) : m_config(config) {}

void RedisServer::start() {

  if (!m_config.replicaOf.empty()) {
    std::istringstream ss(m_config.replicaOf);
    std::string master_host;
    unsigned master_port;
    ss >> master_host >> master_port;

    m_master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_master_fd < 0) {
      throw std::runtime_error("Failed to create master socket");
    }
    struct sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    inet_pton(AF_INET, master_host.c_str(), &master_addr.sin_addr);
    master_addr.sin_port = htons(master_port);

    if (connect(m_master_fd, (struct sockaddr *)&master_addr,
                sizeof(master_addr)) != 0) {
      throw std::runtime_error("Failed to connect to master");
    }

    std::cout << "Client connected" << std::endl;

    std::thread master_thread([this]() {
      ServerConnection master_con(m_master_fd, m_config);
      master_con.handleConnection();
    });
    master_thread.detach();
  }

  setupSocket();
  m_isRunning = true;
  std::cout << "Redis server started at port: " << m_config.port << std::endl;
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
    throw std::runtime_error("Failed to create server socket");
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
  server_addr.sin_port = htons(m_config.port);

  if (bind(m_server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    close(m_server_fd);
    throw std::runtime_error("Failed to bind to port " +
                             std::to_string(m_config.port));
  }

  int connection_backlog = 5;
  if (listen(m_server_fd, connection_backlog) != 0) {
    close(m_server_fd);
    throw std::runtime_error("listen failed");
  }
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

    std::cout << "Client connected" << std::endl;

    std::thread client_thread([client_fd, this]() {
      ClientConnection client_con(client_fd, m_config);
      client_con.handleConnection();
    });
    client_thread.detach();
  }
}

#include "RedisServer.hpp"
#include "ClientConnection.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

RedisServer::RedisServer(const unsigned port) : m_port(port) {}

void RedisServer::start() {
  setupSocket();
  m_isRunning = true;
  std::cout << "Redis server started at port: " << m_port << std::endl;
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
  server_addr.sin_port = htons(m_port);

  if (bind(m_server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    close(m_server_fd);
    throw std::runtime_error("Failed to bind to port " +
                             std::to_string(m_port));
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

    std::thread client_thread([client_fd]() {
      ClientConnection client_con(client_fd);
      client_con.handleClient();
    });
    client_thread.detach();
  }
}

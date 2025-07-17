#include "utils.hpp"
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>

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
  std::cout << "Sent: " << data << " to " << socket_fd << std::endl;
  send(socket_fd, data.c_str(), data.length(), 0);
}

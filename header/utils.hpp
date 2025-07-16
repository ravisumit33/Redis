#pragma once

#include <cstdint>
#include <string>

uint8_t hexCharToInt(char c);
std::string hexToBinary(const std::string &hex);
std::string generateRandomHexId();
std::string readFromSocket(unsigned socket_fd);
void writeToSocket(unsigned socket_fd, const std::string &data);

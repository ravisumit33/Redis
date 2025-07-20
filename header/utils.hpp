#pragma once

#include "Command.hpp"
#include "RespType.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

uint8_t hexCharToInt(char c);
std::string hexToBinary(const std::string &hex);
std::string generateRandomHexId();
std::string readFromSocket(unsigned socket_fd);
void writeToSocket(unsigned socket_fd, const std::string &data);
std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
parseCommand(std::istream &in);

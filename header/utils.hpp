#pragma once

#include "Command.hpp"
#include "RespType.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

uint8_t hexCharToInt(char c);

std::string hexToBinary(const std::string &hex);

std::string generateRandomHexId();

std::string readFromSocket(unsigned socket_fd);

void writeToSocket(unsigned socket_fd, const std::string &data);

std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
parseCommand(std::istream &in);

std::array<std::optional<uint64_t>, 2>
parsePartialStreamEntryId(const std::string &entry_id);

std::array<uint64_t, 2> parseStreamEntryId(const std::string &entry_id);

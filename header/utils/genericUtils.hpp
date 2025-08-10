#pragma once

#include "Command.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
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

std::unique_ptr<RespArray> serializeStreamEntries(
    std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
        entries);

inline uint8_t peekByte(std::istream &in) {
  int ch = in.peek();
  if (ch == std::char_traits<char>::eof()) {
    if (in.eof()) {
      throw std::runtime_error(
          "Unexpected end of file while peeking byte in stream.");
    } else if (in.fail()) {
      throw std::runtime_error("Stream failure while peeking byte.");
    } else {
      throw std::runtime_error("Unknown error while peeking byte.");
    }
  }
  return static_cast<uint8_t>(ch);
}

template <std::size_t N>
std::array<uint8_t, N> readBytes(std::istream &is,
                                 std::endian byte_order = std::endian::big) {
  std::array<uint8_t, N> bytes{};
  if (!is.read(reinterpret_cast<char *>(bytes.data()), bytes.size())) {
    throw std::runtime_error("Failed to read " + std::to_string(N) +
                             " bytes from stream");
  }
  if (byte_order == std::endian::little) {
    std::reverse(bytes.begin(), bytes.end());
  }
  return bytes;
}

template <size_t N>
inline typename std::enable_if_t<(N <= 8), uint64_t>
bytesToUInt(const std::array<uint8_t, N> &bytes_array) {
  uint64_t result = 0;
  for (size_t i = 0; i < N; ++i) {
    result |= static_cast<uint64_t>(bytes_array[i]) << (8 * (N - 1 - i));
  }
  return result;
}

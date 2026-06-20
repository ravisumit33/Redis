#pragma once

#include "Command.hpp"
#include "RespType.hpp"
#include "redis_store/values/StreamValue.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class AppContext;

uint8_t hexCharToInt(char hex_char);

std::string hexToBinary(const std::string &hex);

std::string generateRandomHexId();

std::pair<std::unique_ptr<Command>, std::vector<RespValue>>
parseCommand(std::istream &in_stream, AppContext &context);

std::array<std::optional<uint64_t>, 2>
parsePartialStreamEntryId(const std::string &entry_id);

std::array<uint64_t, 2> parseStreamEntryId(const std::string &entry_id);

RespValue serializeStreamEntries(
    std::vector<std::pair<StreamValue::StreamEntryId, StreamValue::StreamEntry>>
        entries);

inline uint8_t peekByte(std::istream &in_stream) {
  int top_char = in_stream.peek();
  if (top_char == std::char_traits<char>::eof()) {
    if (in_stream.eof()) {
      throw std::runtime_error(
          "Unexpected end of file while peeking byte in stream.");
    }
    if (in_stream.fail()) {
      throw std::runtime_error("Stream failure while peeking byte.");
    }
    throw std::runtime_error("Unknown error while peeking byte.");
  }
  return static_cast<uint8_t>(top_char);
}

template <std::size_t N>
std::array<uint8_t, N> readBytes(std::istream &in_stream,
                                 std::endian byte_order = std::endian::big) {
  std::array<char, N> char_buffer{};
  if (!in_stream.read(char_buffer.data(), char_buffer.size())) {
    throw std::runtime_error("Failed to read " + std::to_string(N) +
                             " bytes from stream");
  }
  std::array<uint8_t, N> bytes{};
  std::transform(
      char_buffer.begin(), char_buffer.end(), bytes.begin(),
      [](char char_byte) { return static_cast<uint8_t>(char_byte); });
  if (byte_order == std::endian::little) {
    std::reverse(bytes.begin(), bytes.end());
  }
  return bytes;
}

template <size_t N>
  requires(N <= sizeof(uint64_t))
inline uint64_t bytesToUInt(const std::array<uint8_t, N> &bytes_array) {
  constexpr size_t kBitsPerByte = 8;
  uint64_t result = 0;
  for (size_t i = 0; i < N; ++i) {
    result |= static_cast<uint64_t>(bytes_array[i])
              << (kBitsPerByte * (N - 1 - i));
  }
  return result;
}

template <typename> inline constexpr bool always_false = false;

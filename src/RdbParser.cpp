#include "RdbParser.hpp"
#include "redis_store/RedisStore.hpp"
#include "utils/genericUtils.hpp"
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <istream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>

namespace {

std::pair<bool, uint64_t> readSizeEncodedValue(std::istream &in_stream) {
  constexpr unsigned ENCODING_TYPE_BIT_SHIFT = 6;
  constexpr uint8_t LOWER_SIX_BITS_MASK = 0x3F;
  constexpr unsigned BYTE_BIT_SHIFT = 8;

  auto [byte] = readBytes<1>(in_stream);
  const unsigned first_two_bits = (byte >> ENCODING_TYPE_BIT_SHIFT);
  switch (first_two_bits) {
  case 0b00:
    return {false, byte & LOWER_SIX_BITS_MASK};
  case 0b01: {
    auto [next_byte] = readBytes<1>(in_stream);
    const uint64_t size =
        (static_cast<uint64_t>(byte) << BYTE_BIT_SHIFT) | next_byte;
    return {false, size};
  }
  case 0b10: {
    const uint64_t size = bytesToUInt(readBytes<4>(in_stream));
    return {false, size};
  }
  case 0b11:
    return {true, byte};
  default:
    throw std::runtime_error(
        "Unrecognized first two bits: " + std::to_string(first_two_bits) +
        " of size encoded value");
  }
}

std::string readString(std::istream &in_stream) {
  constexpr uint64_t ENCODING_8BIT_INT = 0xC0;
  constexpr uint64_t ENCODING_16BIT_INT = 0xC1;
  constexpr uint64_t ENCODING_32BIT_INT = 0xC2;
  constexpr uint64_t ENCODING_LZF = 0xC3;

  auto [is_specially_encoded, size_or_encoding] =
      readSizeEncodedValue(in_stream);
  if (is_specially_encoded) {
    switch (size_or_encoding) {
    case ENCODING_8BIT_INT:
      return std::to_string(
          static_cast<int8_t>(bytesToUInt(readBytes<1>(in_stream))));
    case ENCODING_16BIT_INT:
      return std::to_string(
          static_cast<int16_t>(bytesToUInt(readBytes<2>(in_stream))));
    case ENCODING_32BIT_INT:
      return std::to_string(
          static_cast<int32_t>(bytesToUInt(readBytes<4>(in_stream))));
    case ENCODING_LZF:
      throw std::runtime_error(
          "Unsupported LZF encoding for string encountered.");
    default:
      throw std::runtime_error("Unknown special encoding of string: " +
                               std::to_string(size_or_encoding));
    }
  } else {
    if (size_or_encoding >
        static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max())) {
      throw std::runtime_error("String size too large: " +
                               std::to_string(size_or_encoding));
    }
    std::string str(size_or_encoding, '\0');
    if (!in_stream.read(str.data(),
                        static_cast<std::streamsize>(size_or_encoding))) {
      throw std::runtime_error("Unable to read string from RDB file");
    }
    return str;
  }
}

bool isInlineMetadata(RdbOpcode opcode) {
  static const std::unordered_set<RdbOpcode> inline_metadata_opcodes = {
      RdbOpcode::HASHDATA, RdbOpcode::VALUE_EXPIRETIME,
      RdbOpcode::VALUE_EXPIRETIME_MS, RdbOpcode::STRING_VALUE};
  return inline_metadata_opcodes.contains(opcode);
}

} // namespace

void parseRdb(RdbParser &parser, std::istream &in_stream,
              RdbParserRegistry &registry, RedisStore &store,
              RdbParseState &state) {
  std::visit(
      [&](auto &rdb_parser) {
        rdb_parser.parse(in_stream, registry, store, state);
      },
      parser);
}

void RdbHeaderParser::parse(std::istream &in_stream,
                            RdbParserRegistry & /*registry*/,
                            RedisStore & /*store*/, RdbParseState & /*state*/) {
  constexpr size_t MAGIC_STRING_LENGTH = 5;
  auto magic_string_bytes = readBytes<MAGIC_STRING_LENGTH>(in_stream);
  const std::string magic_string(magic_string_bytes.begin(),
                                 magic_string_bytes.end());
  if (magic_string != "REDIS") {
    throw std::runtime_error("Wrong magic string. Expected: REDIS, found: " +
                             magic_string);
  }
  auto version_num_bytes = readBytes<4>(in_stream);
  const std::string version_num(version_num_bytes.begin(),
                                version_num_bytes.end());
  if (version_num != "0011") {
    throw std::runtime_error("Wrong version number. Expected: 0011, found: " +
                             version_num);
  }
  std::cout << "Magic string: " << magic_string << ", version: " << version_num
            << '\n';
}

void RdbAuxKeyParser::parse(std::istream &in_stream,
                            RdbParserRegistry & /*registry*/,
                            RedisStore & /*store*/, RdbParseState & /*state*/) {
  auto metadata_key = readString(in_stream);
  auto metadata_value = readString(in_stream);
  std::cout << "Aux key: " << metadata_key << ", value: " << metadata_value
            << '\n';
}

void RdbDbParser::parse(std::istream &in_stream, RdbParserRegistry &registry,
                        RedisStore &store, RdbParseState & /*state*/) {
  auto [is_specially_encoded, db_idx] = readSizeEncodedValue(in_stream);
  std::cout << "Database index: " << db_idx << '\n';

  while (isInlineMetadata(static_cast<RdbOpcode>(peekByte(in_stream)))) {
    auto [rdb_opcode] = readBytes<1>(in_stream);
    auto rdb_parser = registry.get(static_cast<RdbOpcode>(rdb_opcode));
    if (!rdb_parser) {
      throw std::runtime_error("Invalid opcode: " + std::to_string(rdb_opcode) +
                               " found during RDB parsing");
    }
    RdbParseState child_state;
    parseRdb(*rdb_parser, in_stream, registry, store, child_state);
  }
}

void RdbHashTableParser::parse(std::istream &in_stream,
                               RdbParserRegistry & /*registry*/,
                               RedisStore & /*store*/,
                               RdbParseState & /*state*/) {
  auto [is_specially_encoded_total, total_kv] = readSizeEncodedValue(in_stream);
  auto [is_specially_encoded_expire, expire_kv] =
      readSizeEncodedValue(in_stream);
  std::cout << "Total key-value pairs: " << total_kv << ", out of which "
            << expire_kv << " has expiry set." << '\n';
}

void RdbEofParser::parse(std::istream &in_stream,
                         RdbParserRegistry & /*registry*/,
                         RedisStore & /*store*/, RdbParseState & /*state*/) {
  constexpr unsigned BYTE_TO_BITS = 8;
  auto check_sum = bytesToUInt(readBytes<BYTE_TO_BITS>(in_stream));
  std::cout << "RDB checksum: " << check_sum << '\n';
}

void RdbStringValueParser::parse(std::istream &in_stream,
                                 RdbParserRegistry & /*registry*/,
                                 RedisStore &store, RdbParseState &state) {
  const std::string key = readString(in_stream);
  const std::string value = readString(in_stream);
  store.setString(key, value, state.expiry);
  std::cout << "Store key: " << key << ", value: " << value;
  if (state.expiry) {
    std::cout << ", expiry after: "
              << duration_cast<std::chrono::milliseconds>(
                     (*state.expiry).time_since_epoch())
                     .count();
  }
  std::cout << '\n';
}

void RdbKeyValueExpMsParser::parse(std::istream &in_stream,
                                   RdbParserRegistry &registry,
                                   RedisStore &store, RdbParseState &state) {
  constexpr unsigned BYTE_TO_BITS = 8;
  auto expiry = std::chrono::milliseconds(
      bytesToUInt(readBytes<BYTE_TO_BITS>(in_stream, std::endian::little)));
  auto [rdb_opcode] = readBytes<1>(in_stream);
  if (!isValue(static_cast<RdbOpcode>(rdb_opcode))) {
    throw std::runtime_error("Invalid opcode: " + std::to_string(rdb_opcode) +
                             " found during RDB parsing");
  }
  auto parser = registry.get(static_cast<RdbOpcode>(rdb_opcode));
  if (!parser) {
    throw std::logic_error("Unexpected: parser not found");
  }
  state.expiry = std::chrono::system_clock::time_point{expiry};
  parseRdb(*parser, in_stream, registry, store, state);
}

void RdbKeyValueExpSParser::parse(std::istream &in_stream,
                                  RdbParserRegistry &registry,
                                  RedisStore &store, RdbParseState &state) {
  constexpr unsigned BYTE_TO_BITS = 8;
  auto expiry = std::chrono::seconds(
      bytesToUInt(readBytes<BYTE_TO_BITS>(in_stream, std::endian::little)));
  auto [rdb_opcode] = readBytes<1>(in_stream);
  if (!isValue(static_cast<RdbOpcode>(rdb_opcode))) {
    throw std::runtime_error("Invalid opcode: " + std::to_string(rdb_opcode) +
                             " found during RDB parsing");
  }
  auto parser = registry.get(static_cast<RdbOpcode>(rdb_opcode));
  if (!parser) {
    throw std::logic_error("Unexpected: parser not found");
  }
  state.expiry = std::chrono::system_clock::time_point{expiry};
  parseRdb(*parser, in_stream, registry, store, state);
}

void registerRdbParsers(RdbParserRegistry &registry) {
  registry.add(RdbOpcode::AUX, [] { return RdbParser{RdbAuxKeyParser{}}; });
  registry.add(RdbOpcode::SELECTDB, [] { return RdbParser{RdbDbParser{}}; });
  registry.add(RdbOpcode::HASHDATA,
               [] { return RdbParser{RdbHashTableParser{}}; });
  registry.add(RdbOpcode::EOF_, [] { return RdbParser{RdbEofParser{}}; });
  registry.add(RdbOpcode::STRING_VALUE,
               [] { return RdbParser{RdbStringValueParser{}}; });
  registry.add(RdbOpcode::VALUE_EXPIRETIME_MS,
               [] { return RdbParser{RdbKeyValueExpMsParser{}}; });
  registry.add(RdbOpcode::VALUE_EXPIRETIME,
               [] { return RdbParser{RdbKeyValueExpSParser{}}; });
}

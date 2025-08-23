#include "RdbParser.hpp"
#include "redis_store/RedisStore.hpp"
#include "utils/genericUtils.hpp"
#include <bit>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <istream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

std::pair<bool, uint64_t>
RdbPayloadParser::readSizeEncodedValue(std::istream &is) {
  auto [byte] = readBytes<1>(is);
  unsigned first_two_bits = (byte >> 6);
  switch (first_two_bits) {
  case 0b00:
    return {false, byte & 0x3F};
  case 0b01: {
    auto [next_byte] = readBytes<1>(is);
    uint64_t size = (static_cast<uint64_t>(byte) << 8) | next_byte;
    return {false, size};
  }
  case 0b10: {
    uint64_t size = bytesToUInt(readBytes<4>(is));
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

std::string RdbPayloadParser::readString(std::istream &is) {
  auto [is_specially_encoded, size_or_encoding] = readSizeEncodedValue(is);
  if (is_specially_encoded) {
    switch (size_or_encoding) {
    case 0xC0:
      return std::to_string(static_cast<int8_t>(bytesToUInt(readBytes<1>(is))));
    case 0xC1:
      return std::to_string(
          static_cast<int16_t>(bytesToUInt(readBytes<2>(is))));
    case 0xC2:
      return std::to_string(
          static_cast<int32_t>(bytesToUInt(readBytes<4>(is))));
    case 0xC3:
      throw std::runtime_error(
          "Unsupported LZF encoding for string encountered.");
    default:
      throw std::runtime_error("Unknown special encoding of string: " +
                               std::to_string(size_or_encoding));
    }
  } else {
    std::string str(size_or_encoding, '\0');
    if (!is.read(str.data(), size_or_encoding)) {
      throw std::runtime_error("Unable to read string from RDB file");
    }
    return str;
  }
}

void RdbHeaderParser::parse(std::istream &is) {
  auto magic_string_bytes = readBytes<5>(is);
  std::string magic_string(magic_string_bytes.begin(),
                           magic_string_bytes.end());
  if (magic_string != "REDIS") {
    throw std::runtime_error("Wrong magic string. Expected: REDIS, found: " +
                             magic_string);
  }
  auto version_num_bytes = readBytes<4>(is);
  std::string version_num(version_num_bytes.begin(), version_num_bytes.end());
  if (version_num != "0011") {
    throw std::runtime_error("Wrong version number. Expected: 0011, found: " +
                             version_num);
  }
  std::cout << "Magic string: " << magic_string << ", version: " << version_num
            << std::endl;
}

RdbParserRegistrar<RdbAuxKeyParser> RdbAuxKeyParser::registrar(RdbOpcode::AUX);

void RdbAuxKeyParser::parse(std::istream &is) {
  auto metadata_key = readString(is);
  auto metadata_value = readString(is);
  std::cout << "Aux key: " << metadata_key << ", value: " << metadata_value
            << std::endl;
}

RdbParserRegistrar<RdbDbParser> RdbDbParser::registrar(RdbOpcode::SELECTDB);

void RdbDbParser::parse(std::istream &is) {
  auto [_, db_idx] = readSizeEncodedValue(is);
  std::cout << "Database index: " << db_idx << std::endl;

  while (isInlineMetadata(static_cast<RdbOpcode>(peekByte(is)))) {
    auto [rdb_opcode] = readBytes<1>(is);
    auto rdb_parser =
        RdbParserRegistry::instance().get(static_cast<RdbOpcode>(rdb_opcode));
    if (!rdb_parser) {
      throw std::runtime_error("Invalid opcode: " + std::to_string(rdb_opcode) +
                               " found during RDB parsing");
    }
    rdb_parser->parse(is);
  }
}

RdbParserRegistrar<RdbHashTableParser>
    RdbHashTableParser::registrar(RdbOpcode::HASHDATA);

void RdbHashTableParser::parse(std::istream &is) {
  auto [_, total_kv] = readSizeEncodedValue(is);
  auto [__, expire_kv] = readSizeEncodedValue(is);
  std::cout << "Total key-value pairs: " << total_kv << ", out of which "
            << expire_kv << " has expiry set." << std::endl;
}

RdbParserRegistrar<RdbEofParser> RdbEofParser::registrar(RdbOpcode::EOF_);

void RdbEofParser::parse(std::istream &is) {
  auto check_sum = bytesToUInt(readBytes<8>(is));
  std::cout << "RDB checksum: " << check_sum << std::endl;
}

RdbParserRegistrar<RdbStringValueParser>
    RdbStringValueParser::registrar(RdbOpcode::STRING_VALUE);

void RdbStringValueParser::parseKeyValue(
    std::istream &is,
    std::optional<std::chrono::system_clock::time_point> expiry) {
  std::string key = readString(is);
  std::string value = readString(is);
  RedisStore::instance().setString(key, value, expiry);
  std::cout << "Store key: " << key << ", value: " << value;
  if (expiry) {
    std::cout << ", expiry after: "
              << duration_cast<std::chrono::milliseconds>(
                     (*expiry).time_since_epoch())
                     .count();
  }
  std::cout << std::endl;
}

void RdbStringValueParser::parse(std::istream &is) { parseKeyValue(is); }

RdbParserRegistrar<RdbKeyValueExpMsParser>
    RdbKeyValueExpMsParser::registrar(RdbOpcode::VALUE_EXPIRETIME_MS);

void RdbKeyValueExpMsParser::parse(std::istream &is) {
  auto expiry = std::chrono::milliseconds(
      bytesToUInt(readBytes<8>(is, std::endian::little)));
  auto [rdb_opcode] = readBytes<1>(is);
  if (!isValue(static_cast<RdbOpcode>(rdb_opcode))) {
    throw std::runtime_error("Invalid opcode: " + std::to_string(rdb_opcode) +
                             " found during RDB parsing");
  }
  auto parser =
      RdbParserRegistry::instance().get(static_cast<RdbOpcode>(rdb_opcode));
  if (!parser) {
    throw std::logic_error("Unexpected: parser not found");
  }
  auto value_parser = static_cast<RdbValueParser *>(parser.get());
  value_parser->parseKeyValue(is,
                              std::chrono::system_clock::time_point{expiry});
}

RdbParserRegistrar<RdbKeyValueExpSParser>
    RdbKeyValueExpSParser::registrar(RdbOpcode::VALUE_EXPIRETIME);

void RdbKeyValueExpSParser::parse(std::istream &is) {
  auto expiry =
      std::chrono::seconds(bytesToUInt(readBytes<8>(is, std::endian::little)));
  auto [rdb_opcode] = readBytes<1>(is);
  if (!isValue(static_cast<RdbOpcode>(rdb_opcode))) {
    throw std::runtime_error("Invalid opcode: " + std::to_string(rdb_opcode) +
                             " found during RDB parsing");
  }
  auto parser =
      RdbParserRegistry::instance().get(static_cast<RdbOpcode>(rdb_opcode));
  if (!parser) {
    throw std::logic_error("Unexpected: parser not found");
  }
  auto value_parser = static_cast<RdbValueParser *>(parser.get());
  value_parser->parseKeyValue(is,
                              std::chrono::system_clock::time_point{expiry});
}

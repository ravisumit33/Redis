#pragma once

#include "Registry.hpp"
#include <chrono>
#include <cstdint>
#include <istream>
#include <optional>
#include <unordered_set>
#include <variant>

class RedisStore;

enum class RdbOpcode : uint8_t {
  AUX = 0xFA,
  SELECTDB = 0xFE,
  EOF_ = 0xFF,
  HASHDATA = 0xFB,
  VALUE_EXPIRETIME_MS = 0xFC,
  VALUE_EXPIRETIME = 0xFD,
  STRING_VALUE = 0x00,
};

inline bool isSectionStart(RdbOpcode opcode) {
  static const std::unordered_set<RdbOpcode> section_start_opcodes = {
      RdbOpcode::AUX, RdbOpcode::SELECTDB, RdbOpcode::EOF_};
  return section_start_opcodes.contains(opcode);
}

inline bool isValue(RdbOpcode opcode) {
  static const std::unordered_set<RdbOpcode> value_opcodes = {
      RdbOpcode::STRING_VALUE, RdbOpcode::VALUE_EXPIRETIME_MS,
      RdbOpcode::VALUE_EXPIRETIME};
  return value_opcodes.contains(opcode);
}

struct RdbParseState {
  std::optional<std::chrono::system_clock::time_point> expiry = std::nullopt;
};

class RdbParserRegistry;

struct RdbHeaderParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbAuxKeyParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbDbParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbHashTableParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbEofParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbStringValueParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbKeyValueExpMsParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

struct RdbKeyValueExpSParser {
  static void parse(std::istream &in_stream, RdbParserRegistry &registry,
                    RedisStore &store, RdbParseState &state);
};

using RdbParser =
    std::variant<RdbHeaderParser, RdbAuxKeyParser, RdbDbParser,
                 RdbHashTableParser, RdbEofParser, RdbStringValueParser,
                 RdbKeyValueExpMsParser, RdbKeyValueExpSParser>;

class RdbParserRegistry : public Registry<RdbOpcode, RdbParser> {};

void parseRdb(RdbParser &parser, std::istream &in_stream,
              RdbParserRegistry &registry, RedisStore &store,
              RdbParseState &state);

void registerRdbParsers(RdbParserRegistry &registry);

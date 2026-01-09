#pragma once

#include "Registry.hpp"
#include <chrono>
#include <cstdint>
#include <istream>
#include <optional>
#include <unordered_set>
#include <utility>

class AppContext;

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
  static std::unordered_set<RdbOpcode> section_start_opcodes = {
      RdbOpcode::AUX, RdbOpcode::SELECTDB, RdbOpcode::EOF_};
  return section_start_opcodes.contains(opcode);
}

inline bool isValue(RdbOpcode opcode) {
  static std::unordered_set<RdbOpcode> value_opcodes = {
      RdbOpcode::STRING_VALUE, RdbOpcode::VALUE_EXPIRETIME_MS,
      RdbOpcode::VALUE_EXPIRETIME};
  return value_opcodes.contains(opcode);
}

struct RdbParseState {
  std::optional<std::chrono::system_clock::time_point> expiry = std::nullopt;
};

class RdbPayloadParser {
public:
  RdbPayloadParser() = default;
  virtual ~RdbPayloadParser() = default;
  RdbPayloadParser(const RdbPayloadParser &) = delete;
  RdbPayloadParser &operator=(const RdbPayloadParser &) = delete;
  RdbPayloadParser(RdbPayloadParser &&) = delete;
  RdbPayloadParser &operator=(RdbPayloadParser &&) = delete;

  virtual void parse(std::istream &in_stream, AppContext &context,
                     RdbParseState &state) = 0;

protected:
  static std::pair<bool, uint64_t>
  readSizeEncodedValue(std::istream &in_stream);

  static std::string readString(std::istream &in_stream);
};

class RdbHeaderParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

class RdbAuxKeyParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

class RdbDbParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;

private:
  static bool isInlineMetadata(RdbOpcode opcode) {
    static std::unordered_set<RdbOpcode> inline_metadata_opcodes = {
        RdbOpcode::HASHDATA, RdbOpcode::VALUE_EXPIRETIME,
        RdbOpcode::VALUE_EXPIRETIME_MS, RdbOpcode::STRING_VALUE};
    return inline_metadata_opcodes.contains(opcode);
  }
};

class RdbHashTableParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

class RdbEofParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

class RdbStringValueParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

class RdbKeyValueExpMsParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

class RdbKeyValueExpSParser : public RdbPayloadParser {
public:
  void parse(std::istream &in_stream, AppContext &context,
             RdbParseState &state) override;
};

using RdbParserRegistry = Registry<RdbOpcode, RdbPayloadParser>;

void registerRdbParsers(RdbParserRegistry &registry);

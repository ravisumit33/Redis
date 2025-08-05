#pragma once

#include "Registrar.hpp"
#include "Registry.hpp"
#include <chrono>
#include <cstdint>
#include <istream>
#include <optional>
#include <unordered_set>
#include <utility>

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

class RdbPayloadParser {
public:
  virtual ~RdbPayloadParser() = default;

  virtual void parse(std::istream &is) = 0;

protected:
  std::pair<bool, uint64_t> readSizeEncodedValue(std::istream &is);

  std::string readString(std::istream &is);
};

class RdbHeaderParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;
};

using RdbParserRegistry = Registry<RdbOpcode, RdbPayloadParser>;

template <typename T>
using RdbParserRegistrar = Registrar<RdbOpcode, RdbPayloadParser, T>;

class RdbAuxKeyParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;

private:
  static RdbParserRegistrar<RdbAuxKeyParser> registrar;
};

class RdbDbParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;

private:
  static RdbParserRegistrar<RdbDbParser> registrar;

  bool isInlineMetadata(RdbOpcode opcode) {
    static std::unordered_set<RdbOpcode> inline_metadata_opcodes = {
        RdbOpcode::HASHDATA, RdbOpcode::VALUE_EXPIRETIME,
        RdbOpcode::VALUE_EXPIRETIME_MS, RdbOpcode::STRING_VALUE};
    return inline_metadata_opcodes.contains(opcode);
  }
};

class RdbHashTableParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;

private:
  static RdbParserRegistrar<RdbHashTableParser> registrar;
};

class RdbEofParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;

private:
  static RdbParserRegistrar<RdbEofParser> registrar;
};

class RdbValueParser : public RdbPayloadParser {
public:
  virtual void parseKeyValue(
      std::istream &is,
      std::optional<std::chrono::system_clock::time_point> expiry) = 0;
};

class RdbStringValueParser : public RdbValueParser {
public:
  virtual void parse(std::istream &is) override;
  void parseKeyValue(std::istream &is,
                     std::optional<std::chrono::system_clock::time_point>
                         expiry = std::nullopt) override;

private:
  static RdbParserRegistrar<RdbStringValueParser> registrar;
};

class RdbKeyValueExpMsParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;

private:
  static RdbParserRegistrar<RdbKeyValueExpMsParser> registrar;
};

class RdbKeyValueExpSParser : public RdbPayloadParser {
public:
  virtual void parse(std::istream &is) override;

private:
  static RdbParserRegistrar<RdbKeyValueExpSParser> registrar;
};

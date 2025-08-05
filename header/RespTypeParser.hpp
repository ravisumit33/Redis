#pragma once

#include "Registrar.hpp"
#include "Registry.hpp"
#include "RespType.hpp"
#include <cstddef>
#include <istream>
#include <memory>

class RespTypeParser {
public:
  virtual ~RespTypeParser() = default;
  virtual std::unique_ptr<RespType> parse(std::istream &in) = 0;
};

using RespParserRegistry = Registry<char, RespTypeParser>;

template <typename T>
using RespParserRegistrar = Registrar<char, RespTypeParser, T>;

class RespStringParser : public RespTypeParser {
public:
  virtual std::unique_ptr<RespType> parse(std::istream &in) override;

private:
  static RespParserRegistrar<RespStringParser> registrar;
};

class RespIntParser : public RespTypeParser {
public:
  virtual std::unique_ptr<RespType> parse(std::istream &in) override;

private:
  static RespParserRegistrar<RespIntParser> registrar;
};

class RespArrayParser : public RespTypeParser {
public:
  virtual std::unique_ptr<RespType> parse(std::istream &in) override;

private:
  static RespParserRegistrar<RespArrayParser> registrar;
};

class RespBulkStringParser : public RespTypeParser {
public:
  virtual std::unique_ptr<RespType> parse(std::istream &in) override;

  void disableLastCrlfParsing() { m_parseLastCrlf = false; }

private:
  static RespParserRegistrar<RespBulkStringParser> registrar;
  bool m_parseLastCrlf = true;
};

class RespErrorParser : public RespTypeParser {
public:
  virtual std::unique_ptr<RespType> parse(std::istream &in) override;

private:
  static RespParserRegistrar<RespErrorParser> registrar;
};

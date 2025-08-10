#include "utils/ArgParser.hpp"
#include <cstddef>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

ArgParser &ArgParser::addOption(const std::string &name,
                                const std::string &help,
                                const std::string &defaultValue) {
  m_options[name] = {help, defaultValue, std::nullopt};
  return *this;
}

ArgParser &ArgParser::addFlag(const std::string &name,
                              const std::string &help) {
  m_flags[name] = {help, false};
  return *this;
}

ArgParser &ArgParser::addPositional(const std::string &name,
                                    const std::string &help) {
  m_positionals.push_back({name, help, std::nullopt});
  return *this;
}

void ArgParser::parse(int argc, char *argv[]) {
  size_t posIdx = 0;
  bool shouldParseOptions = true;

  for (int i = 1; i < argc; ++i) {
    std::string token = argv[i];
    if (shouldParseOptions && token == "--") {
      shouldParseOptions = false;
      continue;
    }

    if (shouldParseOptions && token.starts_with("--")) {
      std::string key = token.substr(2);

      if (m_flags.contains(key)) {
        m_flags[key].value = true;
      } else if (m_options.contains(key)) {
        if (i + 1 >= argc || std::string(argv[i + 1]).starts_with("--")) {
          throw std::runtime_error("Missing value for option: --" + key);
        }
        m_options[key].value = argv[++i];
      } else {
        throw std::runtime_error("Unknown option: --" + key);
      }
    } else {
      if (posIdx >= m_positionals.size()) {
        throw std::runtime_error("Unexpected positional argument: " + token);
      }
      m_positionals[posIdx++].value = token;
    }
  }

  for (const auto &pos : m_positionals) {
    if (!pos.value) {
      throw std::runtime_error("Missing positional argument: " + pos.name);
    }
  }
}

void ArgParser::printHelp(const std::string &progName) const {
  std::cout << "Usage: " << progName;
  for (const auto &[name, _] : m_options) {
    std::cout << " [--" << name << " <val>]";
  }
  for (const auto &[name, _] : m_flags) {
    std::cout << " [--" << name << "]";
  }
  for (const auto &p : m_positionals) {
    std::cout << " <" << p.name << ">";
  }
  std::cout << std::endl;
  std::cout << "\n" << m_description << std::endl;

  if (!m_options.empty()) {
    std::cout << "\nOptions:" << std::endl;
    for (const auto &[name, opt] : m_options) {
      std::cout << " --" << name << "\t" << opt.help
                << " (default: " << opt.defaultValue << ")";
      std::cout << std::endl;
    }
  }

  if (!m_flags.empty()) {
    std::cout << "\nFlags: " << std::endl;
    for (const auto &[name, flag] : m_flags) {
      std::cout << " --" << name << "\t" << flag.help << std::endl;
    }
  }

  if (!m_positionals.empty()) {
    std::cout << "\nPositionals: " << std::endl;
    for (const auto &p : m_positionals) {
      std::cout << " " << p.name << "\t" << p.help << std::endl;
    }
  }
}

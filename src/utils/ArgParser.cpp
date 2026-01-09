#include "utils/ArgParser.hpp"
#include <cstddef>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

ArgParser &ArgParser::addOption(const std::string &name,
                                const std::string &help,
                                const std::string &defaultValue) {
  m_options[name] = {
      .help = help, .defaultValue = defaultValue, .value = std::nullopt};
  return *this;
}

ArgParser &ArgParser::addFlag(const std::string &name,
                              const std::string &help) {
  m_flags[name] = {.help = help, .value = false};
  return *this;
}

ArgParser &ArgParser::addPositional(const std::string &name,
                                    const std::string &help) {
  m_positionals.push_back({name, help, std::nullopt});
  return *this;
}

void ArgParser::parse(std::span<char *> args) {
  size_t posIdx = 0;
  bool shouldParseOptions = true;

  for (size_t i = 1; i < args.size(); ++i) {
    std::string token = args[i];
    if (shouldParseOptions && token == "--") {
      shouldParseOptions = false;
      continue;
    }

    if (shouldParseOptions && token.starts_with("--")) {
      std::string key = token.substr(2);

      if (m_flags.contains(key)) {
        m_flags[key].value = true;
      } else if (m_options.contains(key)) {
        if (i + 1 >= args.size() ||
            std::string(args[i + 1]).starts_with("--")) {
          throw std::runtime_error("Missing value for option: --" + key);
        }
        m_options[key].value = args[++i];
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
  for (const auto &[name, opt] : m_options) {
    std::cout << " [--" << name << " <val>]";
  }
  for (const auto &[name, flag] : m_flags) {
    std::cout << " [--" << name << "]";
  }
  for (const auto &pos : m_positionals) {
    std::cout << " <" << pos.name << ">";
  }
  std::cout << '\n';
  std::cout << "\n" << m_description << '\n';

  if (!m_options.empty()) {
    std::cout << "\nOptions:" << '\n';
    for (const auto &[name, opt] : m_options) {
      std::cout << " --" << name << "\t" << opt.help
                << " (default: " << opt.defaultValue << ")";
      std::cout << '\n';
    }
  }

  if (!m_flags.empty()) {
    std::cout << "\nFlags: " << '\n';
    for (const auto &[name, flag] : m_flags) {
      std::cout << " --" << name << "\t" << flag.help << '\n';
    }
  }

  if (!m_positionals.empty()) {
    std::cout << "\nPositionals: " << '\n';
    for (const auto &pos : m_positionals) {
      std::cout << " " << pos.name << "\t" << pos.help << '\n';
    }
  }
}

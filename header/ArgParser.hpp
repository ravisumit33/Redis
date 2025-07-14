#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class ArgParser {
public:
  ArgParser(std::string desc = "") : m_description(std::move(desc)) {}

  ArgParser &addOption(const std::string &name, const std::string &help = "",
                       const std::string &defaultValue = "");

  ArgParser &addFlag(const std::string &name, const std::string &help = "");

  ArgParser &addPositional(const std::string &name,
                           const std::string &help = "");

  template <typename T> T get(const std::string &name) const;

  void parse(int argc, char *argv[]);

  void printHelp(const std::string &progName) const;

private:
  struct Option {
    std::string help;
    std::string defaultValue;
    std::optional<std::string> value;
  };

  struct Flag {
    std::string help;
    bool value = false;
  };

  struct Positional {
    std::string name;
    std::string help;
    std::optional<std::string> value;
  };

  std::unordered_map<std::string, Option> m_options;
  std::unordered_map<std::string, Flag> m_flags;
  std::vector<Positional> m_positionals;
  std::string m_description;
};

template <> inline bool ArgParser::get(const std::string &name) const {
  if (auto it = m_flags.find(name); it != m_flags.end()) {
    return it->second.value;
  }

  throw std::runtime_error("Argument not found: " + name);
}

template <> inline std::string ArgParser::get(const std::string &name) const {
  if (auto it = m_options.find(name); it != m_options.end()) {
    if (it->second.value) {
      return *(it->second.value);
    }
    return it->second.defaultValue;
  }

  for (const auto &pos : m_positionals) {
    if (pos.name == name) {
      return *pos.value;
    }
  }

  throw std::runtime_error("Argument not found: " + name);
}

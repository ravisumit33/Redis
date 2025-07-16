#pragma once

#include <optional>
#include <string>

struct SlaveConfig {
  std::string master_host;
  unsigned master_port;
};

class AppConfig {
public:
  AppConfig(const unsigned port, const std::optional<SlaveConfig> &sconfig)
      : m_port(port), m_slave_config(sconfig) {}

  unsigned getPort() const { return m_port; }

  const std::optional<SlaveConfig> &getSlaveConfig() const {
    return m_slave_config;
  }

  bool isMaster() const { return getSlaveConfig() == std::nullopt; }

private:
  unsigned m_port;
  std::optional<SlaveConfig> m_slave_config;
};

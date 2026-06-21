#pragma once

#include <optional>
#include <stdexcept>
#include <string>

struct SlaveConfig {
  std::string master_host;
  unsigned master_port;
};

struct MasterConfig {
  std::string dir;
  std::string dbfilename;
};

class AppConfig {
public:
  AppConfig(const unsigned port, const std::optional<MasterConfig> &mconfig,
            const std::optional<SlaveConfig> &sconfig)
      : m_port(port), m_master_config(mconfig), m_slave_config(sconfig) {
    if (m_master_config && m_slave_config) {
      throw std::invalid_argument(
          "Cannot pass arguments for both master & slave");
    }
  }

  unsigned getPort() const { return m_port; }

  const std::optional<SlaveConfig> &getSlaveConfig() const {
    return m_slave_config;
  }

  const std::optional<MasterConfig> &getMasterConfig() const {
    return m_master_config;
  }

  bool isMaster() const { return getMasterConfig() != std::nullopt; }

private:
  unsigned m_port;
  std::optional<SlaveConfig> m_slave_config;
  std::optional<MasterConfig> m_master_config;
};

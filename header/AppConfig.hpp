#pragma once

#include <optional>
#include <string>

struct MasterMetadata {
  std::string master_replid;
  int master_repl_offset;
};

struct SlaveMetadata {
  std::string master_host;
  unsigned master_port;
};

class AppConfig {
public:
  AppConfig(const unsigned port, const std::optional<MasterMetadata> &mmetadata,
            const std::optional<SlaveMetadata> &smetadata)
      : m_port(port), m_master_metadata(mmetadata),
        m_slave_metadata(smetadata) {}

  unsigned getPort() const { return m_port; }

  std::optional<MasterMetadata> getMasterMetadata() const {
    return m_master_metadata;
  }

  std::optional<SlaveMetadata> getSlaveMetadata() const {
    return m_slave_metadata;
  }

private:
  unsigned m_port;
  std::optional<MasterMetadata> m_master_metadata;
  std::optional<SlaveMetadata> m_slave_metadata;
};

#include "AppContext.hpp"
#include "Command.hpp"

AppContext::AppContext(AppConfig config) : m_config(std::move(config)) {}

void AppContext::initialize() {
  registerCommands(m_command_registry);

  if (m_config.isMaster()) {
    m_replication_manager.initAsMaster();
  } else if (m_config.getSlaveConfig()) {
    m_replication_manager.initAsSlave();
  }
}

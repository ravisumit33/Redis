#include "AppContext.hpp"
#include "Command.hpp"
#include "RdbParser.hpp"

AppContext::AppContext(AppConfig config) : m_config(std::move(config)) {
  registerCommands(m_command_registry);
  registerRdbParsers(m_rdb_parser_registry);

  if (m_config.isMaster()) {
    m_replication_manager.initAsMaster();
  } else if (m_config.getSlaveConfig()) {
    m_replication_manager.initAsSlave();
  }
}

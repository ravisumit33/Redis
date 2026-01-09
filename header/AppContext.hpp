#pragma once

#include "AppConfig.hpp"
#include "Command.hpp"
#include "RdbParser.hpp"
#include "RedisChannel.hpp"
#include "ReplicationState.hpp"
#include "redis_store/RedisStore.hpp"

class AppContext {
public:
  explicit AppContext(AppConfig config);
  ~AppContext() = default;

  AppContext(const AppContext &) = delete;
  AppContext &operator=(const AppContext &) = delete;
  AppContext(AppContext &&) = delete;
  AppContext &operator=(AppContext &&) = delete;

  const AppConfig &getConfig() const { return m_config; }
  RedisStore &getRedisStore() { return m_redis_store; }
  const RedisStore &getRedisStore() const { return m_redis_store; }
  ReplicationManager &getReplicationManager() { return m_replication_manager; }
  const ReplicationManager &getReplicationManager() const {
    return m_replication_manager;
  }
  RedisChannelManager &getChannelManager() { return m_channel_manager; }
  const RedisChannelManager &getChannelManager() const {
    return m_channel_manager;
  }
  CommandRegistry &getCommandRegistry() { return m_command_registry; }
  const CommandRegistry &getCommandRegistry() const {
    return m_command_registry;
  }
  RdbParserRegistry &getRdbParserRegistry() { return m_rdb_parser_registry; }
  const RdbParserRegistry &getRdbParserRegistry() const {
    return m_rdb_parser_registry;
  }

private:
  AppConfig m_config;
  RedisStore m_redis_store;
  ReplicationManager m_replication_manager;
  RedisChannelManager m_channel_manager;
  CommandRegistry m_command_registry;
  RdbParserRegistry m_rdb_parser_registry;
};

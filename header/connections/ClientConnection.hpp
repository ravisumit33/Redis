#pragma once

#include "Command.hpp"
#include "Connection.hpp"
#include "RedisChannel.hpp"
#include "RespType.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ClientConnection : public Connection {
public:
  ClientConnection(const unsigned socket_fd, const AppConfig &config)
      : Connection(Type::CLIENT, socket_fd, config), m_is_slave(false) {}

  class RedisSubscriber : public RedisChannel::Subscriber {
  public:
    RedisSubscriber(RedisChannel &ch, ClientConnection *con)
        : m_channel_name(ch.getName()), m_con(con) {}

    void onMessage(const std::string &msg) override;

  private:
    std::string m_channel_name;
    ClientConnection *m_con;
  };

  virtual void handleConnection() override;

  ~ClientConnection();

  void beginTransaction() {
    m_in_transaction = true;
    m_queued_commands.clear();
  }

  bool isInSubscribedMode() { return m_in_subscribed_mode; }

  void queueCommand(std::unique_ptr<Command> cmd,
                    std::vector<std::unique_ptr<RespType>> args) {
    m_queued_commands.emplace_back(std::move(cmd), std::move(args));
  }

  std::unique_ptr<RespType> executeTransaction();

  void discardTransaction() {
    m_queued_commands.clear();
    m_in_transaction = false;
  }

  bool isInTransaction() { return m_in_transaction == true; }

  std::size_t subscribeToChannel(const std::string &channel_name) {
    if (!m_channel_subscriptions.contains(channel_name)) {
      auto channel = RedisChannelManager::instance().getChannel(channel_name);
      auto subscriber =
          RedisChannel::Subscriber::create<RedisSubscriber>(*channel, this);
      m_channel_subscriptions[channel_name] = subscriber;
    }
    m_in_subscribed_mode = true;
    return m_channel_subscriptions.size();
  }

  std::size_t unsubscribeFromChannel(const std::string &channel_name) {
    auto it = m_channel_subscriptions.find(channel_name);
    if (it != m_channel_subscriptions.end()) {
      m_channel_subscriptions.erase(it);
    }
    auto sub_count = m_channel_subscriptions.size();
    if (sub_count == 0) {
      m_in_subscribed_mode = false;
    }
    return sub_count;
  }

private:
  void addAsSlave();

  bool m_is_slave;

  bool m_in_transaction = false;

  bool m_in_subscribed_mode = false;

  std::vector<std::pair<std::unique_ptr<Command>,
                        std::vector<std::unique_ptr<RespType>>>>
      m_queued_commands;

  std::unordered_map<std::string, std::shared_ptr<RedisSubscriber>>
      m_channel_subscriptions;
};

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
  ClientConnection(unsigned socket_fd, AppContext &context)
      : Connection(Type::CLIENT, socket_fd, context) {}

  ClientConnection(const ClientConnection &) = delete;
  ClientConnection &operator=(const ClientConnection &) = delete;

  ClientConnection(ClientConnection &&) = delete;
  ClientConnection &operator=(ClientConnection &&) = delete;

  class RedisSubscriber : public RedisChannel::Subscriber {
  public:
    void onMessage(const std::string &msg) override;

    RedisSubscriber(RedisChannel &chn, ClientConnection &con)
        : m_channel_name(chn.getName()), m_con(con) {}

  private:
    std::string m_channel_name;
    const ClientConnection &m_con;
  };

  void handleConnection() override;

  ~ClientConnection() override;

  void beginTransaction() {
    m_in_transaction = true;
    m_queued_commands.clear();
  }

  bool isInSubscribedMode() const { return m_in_subscribed_mode; }

  void queueCommand(std::unique_ptr<Command> cmd, std::vector<RespValue> args) {
    m_queued_commands.emplace_back(std::move(cmd), std::move(args));
  }

  RespValue executeTransaction();

  void discardTransaction() {
    m_queued_commands.clear();
    m_in_transaction = false;
  }

  bool isInTransaction() const { return m_in_transaction; }

  std::size_t subscribeToChannel(const std::string &channel_name) {
    if (!m_channel_subscriptions.contains(channel_name)) {
      auto *channel = getContext().getChannelManager().getChannel(channel_name);
      m_channel_subscriptions[channel_name] =
          RedisChannel::Subscriber::create<RedisSubscriber>(*channel, *this);
    }
    m_in_subscribed_mode = true;
    return m_channel_subscriptions.size();
  }

  std::size_t unsubscribeFromChannel(const std::string &channel_name) {
    auto itr = m_channel_subscriptions.find(channel_name);
    if (itr != m_channel_subscriptions.end()) {
      m_channel_subscriptions.erase(itr);
    }
    auto sub_count = m_channel_subscriptions.size();
    if (sub_count == 0) {
      m_in_subscribed_mode = false;
    }
    return sub_count;
  }

private:
  void addAsSlave();
  void processCommand(std::unique_ptr<Command> command,
                      std::vector<RespValue> args, const std::string &raw_data);
  void handleSubscribedModeError(const Command &command);
  void checkAndRegisterSlave(const Command &command,
                             const std::vector<RespValue> &args);

  bool m_is_slave{};

  bool m_in_transaction = false;

  bool m_in_subscribed_mode = false;

  std::vector<std::pair<std::unique_ptr<Command>, std::vector<RespValue>>>
      m_queued_commands;

  std::unordered_map<std::string, std::shared_ptr<RedisSubscriber>>
      m_channel_subscriptions;
};

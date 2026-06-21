#pragma once

#include "RedisChannel.hpp"
#include "RespType.hpp"
#include "utils/SocketUtils.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

class Subscriptions {
public:
  explicit Subscriptions(unsigned socket_fd) : m_socket_fd(socket_fd) {}

  class Subscriber : public RedisChannel::Subscriber {
  public:
    Subscriber(RedisChannel &channel, unsigned socket_fd)
        : m_channel_name(channel.getName()), m_socket_fd(socket_fd) {}

    void onMessage(const std::string &msg) override {
      RespArray resp_array;
      resp_array.add(RespBulkString("message"))
          .add(RespBulkString(m_channel_name))
          .add(RespBulkString(msg));
      SocketUtils::writeToSocket(m_socket_fd, resp_array.serialize());
    }

  private:
    std::string m_channel_name;
    unsigned m_socket_fd;
  };

  bool inSubscribedMode() const { return m_in_subscribed_mode; }

  std::size_t subscribe(RedisChannelManager &channels,
                        const std::string &channel_name) {
    if (!m_channel_subscriptions.contains(channel_name)) {
      auto *channel = channels.getChannel(channel_name);
      m_channel_subscriptions[channel_name] =
          RedisChannel::Subscriber::create<Subscriber>(*channel, m_socket_fd);
    }
    m_in_subscribed_mode = true;
    return m_channel_subscriptions.size();
  }

  std::size_t unsubscribe(const std::string &channel_name) {
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
  unsigned m_socket_fd;
  bool m_in_subscribed_mode = false;
  std::unordered_map<std::string, std::shared_ptr<RedisChannel::Subscriber>>
      m_channel_subscriptions;
};

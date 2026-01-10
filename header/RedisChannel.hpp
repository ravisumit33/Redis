#pragma once

#include "utils/ThreadPool.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class RedisChannel : public std::enable_shared_from_this<RedisChannel> {
public:
  using SubscriptionTokenId = uint64_t;

  RedisChannel(std::string channel_name) : m_name(std::move(channel_name)) {}

  class SubscriptionToken;

  class Subscriber {
  public:
    virtual ~Subscriber() = default;
    Subscriber() = default;
    Subscriber(const Subscriber &) = delete;
    Subscriber &operator=(const Subscriber &) = delete;
    Subscriber(Subscriber &&) = delete;
    Subscriber &operator=(Subscriber &&) = delete;

    virtual void onMessage(const std::string &msg) = 0;

    template <typename T, typename... Args>
    static std::shared_ptr<T> create(RedisChannel &redis_ch, Args &&...args) {
      static_assert(std::is_base_of_v<Subscriber, T>,
                    "T must inherit from Subscriber");
      auto sub = std::make_shared<T>(redis_ch, std::forward<Args>(args)...);
      sub->m_token = redis_ch.subscribe(sub);
      return sub;
    }

  private:
    std::shared_ptr<SubscriptionToken> m_token;
  };

  class SubscriptionToken {
  public:
    ~SubscriptionToken() { unsubscribe(); }

    SubscriptionToken(const SubscriptionToken &) = delete;
    SubscriptionToken &operator=(const SubscriptionToken &) = delete;
    SubscriptionToken(SubscriptionToken &&other) noexcept;
    SubscriptionToken &operator=(SubscriptionToken &&other) noexcept;

    void message(const std::string &msg) {
      if (auto subscriber = m_subscriber_weak.lock()) {
        subscriber->onMessage(msg);
      }
    }

    SubscriptionToken(SubscriptionTokenId sub_id,
                      const std::shared_ptr<RedisChannel> &chn,
                      const std::shared_ptr<Subscriber> &sub)
        : m_id(sub_id), m_channel_weak(chn), m_subscriber_weak(sub) {}

  private:
    std::weak_ptr<RedisChannel> m_channel_weak;
    std::weak_ptr<Subscriber> m_subscriber_weak;
    SubscriptionTokenId m_id{};

    void unsubscribe() {
      if (auto channel = m_channel_weak.lock()) {
        channel->unsubscribe(m_id);
      }
    }

    friend class RedisChannel;
  };

  std::size_t subscribersCount() const { return m_subscribers.size(); }

  std::size_t publish(const std::string &msg);

  std::string getName() const { return m_name; }

private:
  std::string m_name;
  std::unordered_map<SubscriptionTokenId, std::weak_ptr<SubscriptionToken>>
      m_subscribers;
  mutable std::shared_mutex m_mutex;
  ThreadPool m_worker_pool;

  SubscriptionTokenId m_next_sub_id = 1;

  std::shared_ptr<SubscriptionToken> subscribe(std::shared_ptr<Subscriber> sub);

  void unsubscribe(SubscriptionTokenId sub_id);

  friend class SubscriptionToken;

  friend class Subscriber;
};

class RedisChannelManager {
public:
  RedisChannelManager() = default;
  ~RedisChannelManager() = default;

  RedisChannelManager(const RedisChannelManager &) = delete;
  RedisChannelManager &operator=(const RedisChannelManager &) = delete;
  RedisChannelManager(RedisChannelManager &&) = delete;
  RedisChannelManager &operator=(RedisChannelManager &&) = delete;

  RedisChannel *getChannel(const std::string &channel_name);

private:
  std::unordered_map<std::string, std::shared_ptr<RedisChannel>> m_channel_map;
};

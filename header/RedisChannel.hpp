#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class RedisChannel {
public:
  RedisChannel(const std::string &channel_name) : m_name(channel_name) {}

  class SubscriptionToken;

  class Subscriber {
  public:
    virtual ~Subscriber() = default;

    virtual void onMessage(const std::string &msg) = 0;

    template <typename T, typename... Args>
    static std::shared_ptr<T> create(RedisChannel &ch, Args &&...args) {
      static_assert(std::is_base_of<Subscriber, T>::value,
                    "T must inherit from Subscriber");
      auto sub = std::shared_ptr<T>(new T(ch, std::forward<Args>(args)...));
      sub->m_token = ch.subscribe(sub);
      return sub;
    }

  private:
    std::shared_ptr<SubscriptionToken> m_token;
  };

  class SubscriptionToken {
  public:
    SubscriptionToken(RedisChannel &ch, std::shared_ptr<Subscriber> sub)
        : m_channel(&ch), m_subscriber(sub) {}

    ~SubscriptionToken() {
      if (m_channel) {
        m_channel->unsubscribe(this);
      }
    }

    SubscriptionToken(const SubscriptionToken &) = delete;
    SubscriptionToken &operator=(const SubscriptionToken &) = delete;
    SubscriptionToken(SubscriptionToken &&) = delete;
    SubscriptionToken &operator=(SubscriptionToken &&) = delete;

    void message(const std::string &msg) {
      if (auto subscriber = m_subscriber.lock()) {
        try {
          subscriber->onMessage(msg);
        } catch (...) {
        }
      }
    }

  private:
    RedisChannel *m_channel;
    std::weak_ptr<Subscriber> m_subscriber;
    friend class RedisChannel;
  };

  std::shared_ptr<SubscriptionToken> subscribe(std::shared_ptr<Subscriber> sub);

  std::size_t subscribersCount() const { return m_subscribers.size(); }

  std::size_t publish(const std::string &message) const;

  std::string getName() const { return m_name; }

private:
  std::string m_name;
  std::unordered_map<Subscriber *, std::weak_ptr<SubscriptionToken>>
      m_subscribers;
  mutable std::mutex m_mutex;

  void unsubscribe(SubscriptionToken *sub_token);

  friend class SubscriptionToken;
};

class RedisChannelManager {
public:
  static RedisChannelManager &instance() {
    static RedisChannelManager manager;
    return manager;
  }

  RedisChannel *getChannel(const std::string &channel_name);

private:
  std::unordered_map<std::string, std::unique_ptr<RedisChannel>> m_channel_map;
};

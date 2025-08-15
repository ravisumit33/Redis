#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class RedisChannel {
public:
  RedisChannel(const std::string &channel_name) : m_name(channel_name) {}

  class SubscriptionToken;

  class Subscriber {
  public:
    virtual ~Subscriber() = default;

    virtual void onMessage(const std::string &msg) = 0;

    template <typename T, typename... Args>
    static std::unique_ptr<T> create(RedisChannel &ch, Args &&...args) {
      static_assert(std::is_base_of<Subscriber, T>::value,
                    "T must inherit from Subscriber");
      // Use direct new instead of make_unique to access private constructor
      auto sub = std::unique_ptr<T>(new T(ch, std::forward<Args>(args)...));
      sub->m_token = ch.subscribe(*sub);
      return sub;
    }

  private:
    std::unique_ptr<SubscriptionToken> m_token;
  };

  class SubscriptionToken {
  public:
    SubscriptionToken(RedisChannel &ch, Subscriber &sub)
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

    void message(const std::string &msg) { m_subscriber.onMessage(msg); }

  private:
    RedisChannel *m_channel;
    Subscriber &m_subscriber;
    friend class RedisChannel;
  };

  std::unique_ptr<SubscriptionToken> subscribe(Subscriber &sub);

  std::size_t subscribersCount() const { return m_subscribers.size(); }

  std::size_t publish(const std::string &message) const;

  std::string getName() const { return m_name; }

private:
  std::string m_name;
  std::vector<SubscriptionToken *> m_subscribers;
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

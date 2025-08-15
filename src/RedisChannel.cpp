#include "RedisChannel.hpp"
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>

RedisChannel::SubscriptionToken::SubscriptionToken(
    SubscriptionToken &&other) noexcept {
  *this = std::move(other);
}

RedisChannel::SubscriptionToken &
RedisChannel::SubscriptionToken::operator=(SubscriptionToken &&other) noexcept {
  if (this != &other) {
    unsubscribe();
    m_id = other.m_id;
    m_subscriber_weak = std::move(other.m_subscriber_weak);
    m_channel_weak = std::move(other.m_channel_weak);
    other.m_channel_weak.reset();
    other.m_subscriber_weak.reset();
  }
  return *this;
}

std::shared_ptr<RedisChannel::SubscriptionToken>
RedisChannel::subscribe(std::shared_ptr<Subscriber> sub) {
  std::lock_guard<std::shared_mutex> lock(m_mutex);
  SubscriptionTokenId token_id = m_next_sub_id++;
  auto sub_token = SubscriptionToken::create(token_id, shared_from_this(), sub);
  m_subscribers[token_id] = sub_token;
  return sub_token;
}

void RedisChannel::unsubscribe(SubscriptionTokenId id) {
  std::lock_guard<std::shared_mutex> lock(m_mutex);
  m_subscribers.erase(id);
}

std::size_t RedisChannel::publish(const std::string &msg) {
  std::vector<std::weak_ptr<SubscriptionToken>> active_subscriptions;
  std::vector<SubscriptionTokenId> inactive_subscriptions;
  {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    active_subscriptions.reserve(m_subscribers.size());
    for (auto &[sub_id, sub_token_weak] : m_subscribers) {
      if (auto sub_token_ptr = sub_token_weak.lock()) {
        active_subscriptions.push_back(sub_token_ptr);
      } else {
        inactive_subscriptions.push_back(sub_id);
      }
    }
  }

  {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    for (auto sub_id : inactive_subscriptions) {
      m_subscribers.erase(sub_id);
    }
  }

  for (auto &subscription_token_weak : active_subscriptions) {
    m_worker_pool.submit([subscription_token_weak, msg]() {
      if (auto subscription_token = subscription_token_weak.lock()) {
        if (auto subscriber = subscription_token->m_subscriber_weak.lock()) {
          try {
            subscriber->onMessage(msg);
          } catch (const std::exception &ex) {
            std::cerr << "Subscriber threw exception while publishing message: "
                      << ex.what() << std::endl;
          } catch (...) {
            std::cerr
                << "Subscriber threw unknown exception while publishing message"
                << std::endl;
          }
        }
      }
    });
  }

  return active_subscriptions.size();
}

RedisChannel *RedisChannelManager::getChannel(const std::string &channel_name) {
  auto it = m_channel_map.find(channel_name);
  if (it != m_channel_map.end()) {
    return it->second.get();
  }
  auto channel_ptr = std::make_shared<RedisChannel>(channel_name);
  return (m_channel_map[channel_name] = channel_ptr).get();
}

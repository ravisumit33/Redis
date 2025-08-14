#include "RedisChannel.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>

std::shared_ptr<RedisChannel::SubscriptionToken>
RedisChannel::subscribe(std::shared_ptr<Subscriber> sub) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_subscribers.find(sub.get());
  std::shared_ptr<SubscriptionToken> sub_token;
  if (it != m_subscribers.end() && (sub_token = it->second.lock())) {
    return sub_token;
  }
  sub_token = std::make_shared<SubscriptionToken>(*this, sub);
  m_subscribers[sub.get()] = sub_token;
  return sub_token;
}

void RedisChannel::unsubscribe(SubscriptionToken *sub_token) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto subscriber = sub_token->m_subscriber.lock();
  if (subscriber) {
    auto it = m_subscribers.find(subscriber.get());
    if (it != m_subscribers.end()) {
      m_subscribers.erase(it);
    }
  } else {
    std::erase_if(m_subscribers, [sub_token](auto &pair) {
      if (pair.second.lock().get() == sub_token) {
        return true;
      }
      return false;
    });
  }
  sub_token->m_channel = nullptr;
}

std::size_t RedisChannel::publish(const std::string &msg) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &[_, sub_token_weak] : m_subscribers) {
    if (auto sub_token = sub_token_weak.lock()) {
      sub_token->message(msg);
    } else {
      throw std::logic_error("Stale subscription");
    }
  }
  return m_subscribers.size();
}

RedisChannel *RedisChannelManager::getChannel(const std::string &channel_name) {
  auto it = m_channel_map.find(channel_name);
  if (it != m_channel_map.end()) {
    return it->second.get();
  }
  auto channel_ptr = std::make_unique<RedisChannel>(channel_name);
  return (m_channel_map[channel_name] = std::move(channel_ptr)).get();
}

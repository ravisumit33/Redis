#include "RedisChannel.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>

std::unique_ptr<RedisChannel::SubscriptionToken>
RedisChannel::subscribe(Subscriber &sub) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto sub_token = std::make_unique<SubscriptionToken>(*this, sub);
  m_subscribers.push_back(sub_token.get());
  return sub_token;
}

void RedisChannel::unsubscribe(SubscriptionToken *sub_token) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_subscribers.begin(), m_subscribers.end(), sub_token);
  if (it != m_subscribers.end()) {
    m_subscribers.erase(it);
  }
  sub_token->m_channel = nullptr;
}

std::size_t RedisChannel::publish(const std::string &msg) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto sub_token : m_subscribers) {
    sub_token->message(msg);
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

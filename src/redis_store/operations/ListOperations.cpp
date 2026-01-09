#include "redis_store/RedisStore.hpp"
#include "redis_store/values/ListValue.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

std::size_t
RedisStore::addListElementsAtEnd(const std::string &key,
                                 const std::vector<std::string> &elements) {
  auto store = writeStore();
  auto itr = store->find(key);

  std::size_t ret = elements.size();
  if (itr == store->end()) {
    auto val = ListValue(elements);
    store->insert_or_assign(key, val);
  } else {
    auto *list = std::get_if<ListValue>(&itr->second);
    if (list == nullptr) {
      throw std::runtime_error("Key doesn't correspond to LIST value type");
    }
    ret += list->size();
    list->insertAtEnd(elements);
  }
  notifyBlockingClients(key);
  return ret;
}

std::size_t
RedisStore::addListElementsAtBegin(const std::string &key,
                                   const std::vector<std::string> &elements) {
  auto store = writeStore();
  auto itr = store->find(key);

  std::size_t ret = elements.size();
  if (itr == store->end()) {
    auto val = ListValue(elements);
    store->insert_or_assign(key, val);
  } else {
    auto *list = std::get_if<ListValue>(&itr->second);
    if (list == nullptr) {
      throw std::runtime_error("Key doesn't correspond to LIST value type");
    }
    ret += list->size();
    list->insertAtBegin(elements);
  }
  notifyBlockingClients(key);
  return ret;
}

bool RedisStore::waitForListElements(
    const std::string &key, unsigned int &count, double timeout_s,
    const std::function<void()> &pop_elements) const {
  auto wait_token = getBlockingQueue(key)->create_wait_token();

  auto has_elements = [&]() {
    auto store = readStore();
    auto itr = store->find(key);
    if (itr == store->end()) {
      return false;
    }
    const auto *list = std::get_if<ListValue>(&itr->second);
    return list != nullptr && !list->empty();
  };

  if (timeout_s == 0) {
    while (count != 0U) {
      wait_token->wait(has_elements);
      pop_elements();
    }
    return false;
  }

  constexpr uint16_t ms_in_s = 1000;
  auto timeout_duration =
      std::chrono::milliseconds(static_cast<uint64_t>(timeout_s * ms_in_s));
  auto start_time = std::chrono::steady_clock::now();

  while (count != 0U) {
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    auto remaining =
        timeout_duration -
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    if (remaining <= std::chrono::milliseconds(0)) {
      std::cerr << "Deadline of " << timeout_s << " s reached" << '\n';
      return true;
    }

    if (!wait_token->wait_for(remaining, has_elements)) {
      std::cerr << "Deadline of " << timeout_s << " s reached" << '\n';
      return true;
    }

    pop_elements();
  }
  return false;
}

std::pair<bool, std::vector<std::string>>
RedisStore::removeListElementsAtBegin(const std::string &key,
                                      unsigned int count,
                                      std::optional<double> timeout_s) {

  std::vector<std::string> popped_elements;
  auto pop_elements = [&]() {
    auto store = writeStore();
    auto itr = store->find(key);
    if (itr == store->end()) {
      return;
    }
    auto *list = std::get_if<ListValue>(&itr->second);
    if (!list) {
      throw std::runtime_error("Key doesn't correspond to LIST value type");
    }
    while (!list->empty() && count--) {
      popped_elements.push_back(list->pop_front());
    }
  };

  pop_elements();

  bool timed_out = false;
  if ((count != 0U) && timeout_s) {
    timed_out = waitForListElements(key, count, *timeout_s, pop_elements);
  }
  return {timed_out, popped_elements};
}

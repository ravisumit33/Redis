#include "redis_store/RedisStore.hpp"
#include "redis_store/values/ListValue.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

std::size_t
RedisStore::addListElementsAtEnd(const std::string &key,
                                 const std::vector<std::string> &elements) {
  auto store = writeStore();
  auto it = store->find(key);

  std::size_t ret = elements.size();
  if (it == store->end()) {
    auto val = std::make_unique<ListValue>(elements);
    store->insert_or_assign(key, std::move(val));
  } else {
    auto &list = static_cast<ListValue &>(*it->second);
    ret += list.size();
    list.insertAtEnd(elements);
  }
  notifyBlockingClients(key);
  return ret;
}

std::size_t
RedisStore::addListElementsAtBegin(const std::string &key,
                                   const std::vector<std::string> &elements) {
  auto store = writeStore();
  auto it = store->find(key);

  std::size_t ret = elements.size();
  if (it == store->end()) {
    auto val = std::make_unique<ListValue>(elements);
    store->insert_or_assign(key, std::move(val));
  } else {
    auto &list = static_cast<ListValue &>(*it->second);
    ret += list.size();
    list.insertAtBegin(elements);
  }
  notifyBlockingClients(key);
  return ret;
}

std::pair<bool, std::vector<std::string>>
RedisStore::removeListElementsAtBegin(const std::string &key,
                                      unsigned int count,
                                      std::optional<double> timeout_s) {

  std::vector<std::string> popped_elements;
  auto pop_elements = [&]() {
    auto store = readStore();
    auto it = store->find(key);
    if (it == store->end()) {
      return;
    }
    auto &list = static_cast<ListValue &>(*it->second);
    while (!list.empty() && count--) {
      popped_elements.push_back(list.pop_front());
    }
  };

  pop_elements();

  bool timed_out = false;
  if (count && timeout_s) {
    auto wait_token = getBlockingQueue(key)->create_wait_token();

    auto has_elements = [&]() {
      auto store = readStore();
      auto it = store->find(key);
      if (it == store->end()) {
        return false;
      }
      auto &list = static_cast<ListValue &>(*it->second);
      bool ret = !list.empty();
      return ret;
    };

    if (*timeout_s == 0) {
      while (count) {
        wait_token->wait(has_elements);
        pop_elements();
      }
    } else {
      auto timeout_duration =
          std::chrono::milliseconds(static_cast<uint64_t>(*timeout_s * 1000));
      auto start_time = std::chrono::steady_clock::now();

      while (count) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto remaining =
            timeout_duration -
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

        if (remaining <= std::chrono::milliseconds(0)) {
          std::cerr << "Deadline of " << *timeout_s << " s reached"
                    << std::endl;
          timed_out = true;
          break;
        }

        if (!wait_token->wait_for(remaining, has_elements)) {
          std::cerr << "Deadline of " << *timeout_s << " s reached"
                    << std::endl;
          timed_out = true;
          break;
        }

        pop_elements();
      }
    }
  }
  return {timed_out, popped_elements};
}

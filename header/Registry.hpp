#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename Key, typename Value> class Registry {
public:
  using Factory = std::function<std::unique_ptr<Value>()>;

  Registry() = default;
  ~Registry() = default;

  Registry(const Registry &) = delete;
  Registry &operator=(const Registry &) = delete;
  Registry(Registry &&) = delete;
  Registry &operator=(Registry &&) = delete;

  void add(const Key &key, Factory factory) {
    std::lock_guard<std::shared_mutex> lock(mMutex);
    mFactoryMap[key] = factory;
  }

  std::unique_ptr<Value> get(const Key &key) {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if (!mFactoryMap.contains(key)) {
      return nullptr;
    }
    auto factory = mFactoryMap[key];
    return factory();
  }

private:
  std::map<Key, Factory> mFactoryMap;
  std::shared_mutex mMutex;
};

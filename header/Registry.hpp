#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename Key, typename Value> class Registry {
public:
  using Factory = std::function<std::unique_ptr<Value>()>;

  void add(const Key &k, Factory factory) {
    std::lock_guard<std::shared_mutex> lock(mMutex);
    mFactoryMap[k] = factory;
  }

  std::unique_ptr<Value> get(const Key &k) {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if (!mFactoryMap.contains(k)) {
      return nullptr;
    }
    auto factory = mFactoryMap[k];
    return factory();
  }

  static Registry &instance() {
    static Registry registry;
    return registry;
  }

private:
  Registry() = default;
  std::map<Key, Factory> mFactoryMap;
  std::shared_mutex mMutex;
};

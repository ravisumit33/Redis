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

  Value *get(const Key &k) {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if (mInstanceMap.contains(k)) {
      return mInstanceMap[k].get();
    }
    if (!mFactoryMap.contains(k)) {
      return nullptr;
    }
    auto factory = mFactoryMap[k];
    return (mInstanceMap[k] = std::move(factory())).get();
  }

  static Registry &instance() {
    static Registry registry;
    return registry;
  }

private:
  Registry() = default;
  std::map<Key, Factory> mFactoryMap;
  std::map<Key, std::unique_ptr<Value>> mInstanceMap;
  std::shared_mutex mMutex;
};

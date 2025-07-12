#pragma once
#include <functional>
#include <map>
#include <memory>

template <typename Key, typename Value> class Registry {
public:
  using Factory = std::function<std::unique_ptr<Value>()>;

  void add(const Key &k, Factory factory) { mFactoryMap[k] = factory; }

  std::unique_ptr<Value> get(const Key &k) {
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
};

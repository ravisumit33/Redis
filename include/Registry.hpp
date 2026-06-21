#pragma once

#include <functional>
#include <map>
#include <optional>

template <typename Key, typename Value> class Registry {
public:
  using Factory = std::function<Value()>;

  Registry() = default;
  ~Registry() = default;

  Registry(const Registry &) = delete;
  Registry &operator=(const Registry &) = delete;
  Registry(Registry &&) = delete;
  Registry &operator=(Registry &&) = delete;

  void add(const Key &key, Factory factory) {
    mFactoryMap[key] = std::move(factory);
  }

  std::optional<Value> get(const Key &key) const {
    auto iter = mFactoryMap.find(key);
    if (iter == mFactoryMap.end()) {
      return std::nullopt;
    }
    return iter->second();
  }

private:
  std::map<Key, Factory> mFactoryMap;
};

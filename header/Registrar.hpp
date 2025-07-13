#pragma once

#include "Registry.hpp"
#include <memory>

template <typename Key, typename BaseType, typename DerivedType>
class Registrar {
  static_assert(std::is_base_of<BaseType, DerivedType>::value,
                "Derived must be a subclass of Base");

public:
  Registrar(const Key &key) {
    Registry<Key, BaseType>::instance().add(
        key, []() { return std::make_unique<DerivedType>(); });
  }
};

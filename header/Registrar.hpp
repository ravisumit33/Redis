#pragma once

#include "Registry.hpp"
#include <memory>

// NOTE: This registrar is currently not getting used due to issues in static
// registry pattern e.g. static initialization order fiasco, chances of
// exception before main, etc.
template <typename Key, typename BaseType, typename DerivedType>
class Registrar {
  static_assert(std::is_base_of_v<BaseType, DerivedType>,
                "Derived must be a subclass of Base");

public:
  Registrar(const Key &key) {
    Registry<Key, BaseType>::instance().add(
        key, []() { return std::make_unique<DerivedType>(); });
  }
};

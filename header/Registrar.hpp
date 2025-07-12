#pragma once

#include "Registry.hpp"
#include <memory>

template <typename Key, typename BaseType, typename DerivedType>
class Registrar {
public:
  Registrar(const Key &key) {
    Registry<Key, BaseType>::instance().add(
        key, []() { return std::make_unique<DerivedType>(); });
  }
};

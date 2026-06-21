#pragma once

#include <concepts>

class AppConfig;
class RedisStore;
class RedisChannelManager;
class ReplicationManager;
class TransactionState;
class Subscriptions;

template <typename C>
concept HasStore = requires(C &ctx) {
  { ctx.store() } -> std::same_as<RedisStore &>;
};

template <typename C>
concept Configurable = requires(C &ctx) {
  { ctx.config() } -> std::same_as<const AppConfig &>;
};

template <typename C>
concept Channels = requires(C &ctx) {
  { ctx.channels() } -> std::same_as<RedisChannelManager &>;
};

template <typename C>
concept Replicating = requires(C &ctx) {
  { ctx.replication() } -> std::same_as<ReplicationManager &>;
};

template <typename C>
concept PubSub = requires(C &ctx) {
  { ctx.subs() } -> std::same_as<Subscriptions &>;
};

template <typename C>
concept Transactional = requires(C &ctx) {
  { ctx.txn() } -> std::same_as<TransactionState &>;
};

#pragma once

#include "AppContext.hpp"
#include "connections/Capabilities.hpp"
#include "connections/Subscriptions.hpp"
#include "connections/TransactionState.hpp"

class ClientExecContext {
public:
  ClientExecContext(AppContext &context, unsigned socket_fd,
                    TransactionState &transaction, Subscriptions &subscriptions)
      : m_context(context), m_transaction(transaction),
        m_subscriptions(subscriptions), m_socket_fd(socket_fd) {}

  ClientExecContext(const ClientExecContext &) = delete;
  ClientExecContext &operator=(const ClientExecContext &) = delete;
  ClientExecContext(ClientExecContext &&) = delete;
  ClientExecContext &operator=(ClientExecContext &&) = delete;
  ~ClientExecContext() = default;

  unsigned getSocketFd() const { return m_socket_fd; }

  RedisStore &store() { return m_context.getRedisStore(); }
  RedisChannelManager &channels() { return m_context.getChannelManager(); }
  ReplicationManager &replication() {
    return m_context.getReplicationManager();
  }
  const AppConfig &config() { return m_context.getConfig(); }
  TransactionState &txn() { return m_transaction; }
  Subscriptions &subs() { return m_subscriptions; }

private:
  AppContext &m_context;
  TransactionState &m_transaction;
  Subscriptions &m_subscriptions;
  unsigned m_socket_fd;
};

class ReplicaLinkExecContext {
public:
  ReplicaLinkExecContext(AppContext &context, unsigned socket_fd)
      : m_context(context), m_socket_fd(socket_fd) {}

  ReplicaLinkExecContext(const ReplicaLinkExecContext &) = delete;
  ReplicaLinkExecContext &operator=(const ReplicaLinkExecContext &) = delete;
  ReplicaLinkExecContext(ReplicaLinkExecContext &&) = delete;
  ReplicaLinkExecContext &operator=(ReplicaLinkExecContext &&) = delete;
  ~ReplicaLinkExecContext() = default;

  unsigned getSocketFd() const { return m_socket_fd; }

  RedisStore &store() { return m_context.getRedisStore(); }
  RedisChannelManager &channels() { return m_context.getChannelManager(); }
  ReplicationManager &replication() {
    return m_context.getReplicationManager();
  }
  const AppConfig &config() { return m_context.getConfig(); }

private:
  AppContext &m_context;
  unsigned m_socket_fd;
};

static_assert(HasStore<ClientExecContext> && Configurable<ClientExecContext> &&
              Channels<ClientExecContext> && Replicating<ClientExecContext> &&
              PubSub<ClientExecContext> && Transactional<ClientExecContext>);
static_assert(HasStore<ReplicaLinkExecContext> &&
              Configurable<ReplicaLinkExecContext> &&
              Channels<ReplicaLinkExecContext> &&
              Replicating<ReplicaLinkExecContext> &&
              !PubSub<ReplicaLinkExecContext> &&
              !Transactional<ReplicaLinkExecContext>);

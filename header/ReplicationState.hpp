#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <variant>

class MasterState {
public:
  MasterState() = default;
  MasterState(const std::string &replid, const uint64_t repl_offset = 0)
      : m_replid(replid), m_repl_offset(repl_offset) {}

  std::string getReplId() const { return m_replid; }

  uint64_t getReplOffset() const { return m_repl_offset; }

  bool hasSlaves() const { return !m_slaves.empty(); }

  void addSlave(unsigned slave_fd) {
    std::lock_guard<std::mutex> lock(mMutex);
    m_slaves.insert(slave_fd);
  }

  void removeSlave(unsigned slave_fd) {
    std::lock_guard<std::mutex> lock(mMutex);
    m_slaves.erase(slave_fd);
  }

  void propagateToSlave(const std::string &data);

private:
  std::string m_replid;
  uint64_t m_repl_offset;
  std::unordered_set<unsigned> m_slaves;
  mutable std::mutex mMutex;
};

class SlaveState {
public:
  SlaveState() = default;
};

class ReplicationManager {
public:
  enum class Role { None, Master, Slave };

  static ReplicationManager &getInstance() {
    static ReplicationManager instance;
    return instance;
  }

  void initAsMaster();
  void initAsSlave();

  Role getRole() const { return m_role; }

  MasterState &master();
  SlaveState &slave();

private:
  Role m_role = Role::None;
  bool m_initialized = false;
  std::variant<MasterState, SlaveState> m_state;

  ReplicationManager() = default;
  ReplicationManager(const ReplicationManager &) = delete;
  ReplicationManager &operator=(const ReplicationManager &) = delete;
};

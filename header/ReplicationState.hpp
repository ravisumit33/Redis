#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>

class MasterState {
public:
  MasterState() = default;
  MasterState(const std::string &replid, const uint64_t repl_offset = 0)
      : m_replid(replid), m_repl_offset(repl_offset) {}

  std::string getReplId() const { return m_replid; }

  uint64_t getReplOffset() const { return m_repl_offset; }

  void updateReplOffset(uint64_t delta) {
    m_repl_offset += delta;
    std::cout << "Master repl_offset updated to: " << m_repl_offset
              << std::endl;
  }

  bool hasSlaves() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_slaves.empty();
  }

  unsigned getSlaveCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_slaves.size();
  }

  void addSlave(unsigned slave_fd, uint64_t initial_offset = 0) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_slaves[slave_fd] = initial_offset;
    m_cv.notify_all();
  }

  void removeSlave(unsigned slave_fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_slaves.erase(slave_fd);
    m_cv.notify_all();
  }

  void updateSlave(unsigned slave_fd, uint64_t offset);

  void propagateToSlave(const std::string &data);

  unsigned waitForSlaves(unsigned expected_slaves_count, uint64_t timeout_ms);

  void sendGetAckToSlaves();

private:
  std::string m_replid;
  uint64_t m_repl_offset;
  std::unordered_map<unsigned, uint64_t> m_slaves;
  mutable std::mutex m_mutex;
  std::condition_variable m_cv;

  MasterState(const MasterState &state) = delete;

  MasterState &operator=(const MasterState &) = delete;
};

class SlaveState {
public:
  SlaveState() = default;

  void addBytesProcessed(std::size_t bytes) { m_bytes_processed += bytes; }

  std::size_t getBytesProcessed() const { return m_bytes_processed; }

private:
  std::size_t m_bytes_processed = 0;

  SlaveState(const SlaveState &state) = delete;

  SlaveState &operator=(const SlaveState &) = delete;
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
  std::variant<std::shared_ptr<MasterState>, std::shared_ptr<SlaveState>>
      m_state;

  ReplicationManager() = default;
  ReplicationManager(const ReplicationManager &) = delete;
  ReplicationManager &operator=(const ReplicationManager &) = delete;
};

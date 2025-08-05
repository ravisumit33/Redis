#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils.hpp"
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

void MasterState::propagateToSlave(const std::string &data) {
  if (!hasSlaves()) {
    return;
  }
  std::cout << "Propagating write command to " << getSlaveCount() << " slaves"
            << std::endl;
  for (const auto [slave_fd, _] : m_slaves) {
    writeToSocket(slave_fd, data);
  }
}

void MasterState::sendGetAckToSlaves() {
  std::vector<unsigned> curr_slave_fds;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto [slave_fd, _] : m_slaves) {
      curr_slave_fds.push_back(slave_fd);
    }
  }

  for (const unsigned slave_fd : curr_slave_fds) {
    auto array = RespArray();
    array.add(std::make_unique<RespBulkString>("REPLCONF"));
    array.add(std::make_unique<RespBulkString>("GETACK"));
    array.add(std::make_unique<RespBulkString>("*"));
    writeToSocket(slave_fd, array.serialize());
  }
}

void MasterState::updateSlave(unsigned slave_fd, uint64_t offset) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_slaves.find(slave_fd);
  if (it != m_slaves.end()) {
    it->second = offset;
    m_cv.notify_all();
    std::cout << "Slave [fd=" << slave_fd << "] updated with offset: " << offset
              << std::endl;
  }
}

unsigned MasterState::waitForSlaves(unsigned expected_slaves_count,
                                    uint64_t timeout_ms) {
  std::unique_lock<std::mutex> lock(m_mutex);
  auto deadline =
      std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);

  auto count_done_slaves = [this] -> unsigned {
    unsigned count = 0;
    for (const auto [slave_fd, offset] : m_slaves) {
      if (offset >= m_repl_offset) {
        ++count;
      }
    }
    return count;
  };

  while (true) {
    unsigned done_slaves_count = count_done_slaves();
    if (done_slaves_count >= expected_slaves_count) {
      return done_slaves_count;
    }
    if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout) {
      std::cout << "Dealine of " << timeout_ms << " ms reached" << std::endl;
      return count_done_slaves();
    }
  }
}

void ReplicationManager::initAsMaster() {
  if (m_initialized) {
    throw std::logic_error("Already initialized");
  }
  m_state.emplace<std::shared_ptr<MasterState>>(
      std::make_shared<MasterState>(generateRandomHexId()));
  m_role = Role::Master;
  m_initialized = true;
}

void ReplicationManager::initAsSlave() {
  if (m_initialized) {
    throw std::logic_error("Already initialized");
  }
  m_state.emplace<std::shared_ptr<SlaveState>>(std::make_shared<SlaveState>());
  m_role = Role::Slave;
  m_initialized = true;
}

MasterState &ReplicationManager::master() {
  if (m_role != Role::Master) {
    throw std::logic_error("Not a master");
  }
  return *std::get<std::shared_ptr<MasterState>>(m_state);
}

SlaveState &ReplicationManager::slave() {
  if (m_role != Role::Slave) {
    throw std::logic_error("Not a slave");
  }
  return *std::get<std::shared_ptr<SlaveState>>(m_state);
}

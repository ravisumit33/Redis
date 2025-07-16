#include "ReplicationState.hpp"
#include "utils.hpp"
#include <mutex>
#include <stdexcept>

void MasterState::propagateToSlave(const std::string &data) {
  std::lock_guard<std::mutex> lock(mMutex);
  if (!hasSlaves()) {
    throw std::logic_error("No slaves to propagate");
  }
  for (const auto slave : m_slaves) {
    writeToSocket(slave, data);
  }
}

void ReplicationManager::initAsMaster() {
  if (m_initialized) {
    throw std::logic_error("Already initialized");
  }
  m_state.emplace<MasterState>(generateRandomHexId());
  m_role = Role::Master;
}

void ReplicationManager::initAsSlave() {
  if (m_initialized) {
    throw std::logic_error("Already initialized");
  }
  m_state.emplace<SlaveState>();
  m_role = Role::Slave;
}

MasterState &ReplicationManager::master() {
  if (m_role != Role::Master) {
    throw std::logic_error("Not a master");
  }
  return std::get<MasterState>(m_state);
}

SlaveState &ReplicationManager::slave() {
  if (m_role != Role::Slave) {
    throw std::logic_error("Not a slave");
  }
  return std::get<SlaveState>(m_state);
}

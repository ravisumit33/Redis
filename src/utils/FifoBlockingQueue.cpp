#include "utils/FifoBlockingQueue.hpp"
#include <algorithm>
#include <cstddef>

FifoBlockingQueue::WaitToken::WaitToken(FifoBlockingQueue &queue)
    : m_queue(&queue) {}

FifoBlockingQueue::WaitToken::~WaitToken() {
  if (m_queue) {
    m_queue->remove_wait_token(this);
  }
}

std::unique_ptr<FifoBlockingQueue::WaitToken>
FifoBlockingQueue::create_wait_token() {
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  auto token = std::make_unique<WaitToken>(*this);
  m_waiting_list.push_back(token.get());
  return token;
}

void FifoBlockingQueue::notify_one() {
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  if (!m_waiting_list.empty()) {
    WaitToken *token = m_waiting_list.front();
    token->notify();
  }
}

void FifoBlockingQueue::notify_all() {
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  for (WaitToken *token : m_waiting_list) {
    token->notify();
  }
}

std::size_t FifoBlockingQueue::waiting_count() const {
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  return m_waiting_list.size();
}

void FifoBlockingQueue::remove_wait_token(WaitToken *token) {
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  auto it = std::find(m_waiting_list.begin(), m_waiting_list.end(), token);
  if (it != m_waiting_list.end()) {
    m_waiting_list.erase(it);
  }
  token->m_queue = nullptr;
}

FifoBlockingQueue::~FifoBlockingQueue() {
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  for (auto token : m_waiting_list) {
    (*token).m_queue = nullptr;
  }
}

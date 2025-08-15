#include "utils/ThreadPool.hpp"
#include <cstddef>

ThreadPool::ThreadPool(std::size_t pool_size) {
  for (std::size_t i = 0; i < pool_size; ++i) {
    m_workers.emplace_back([this] {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(m_mutex);
          m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
          if (m_stop)
            return;
          task = std::move(m_tasks.front());
          m_tasks.pop();
        }
        try {
          task();
        } catch (...) {
          // swallow
        }
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_stop = true;
  }
  m_cv.notify_all();
  for (auto &t : m_workers)
    if (t.joinable())
      t.join();
}

bool ThreadPool::submit(std::function<void()> f) {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_stop) {
      return false;
    }
    m_tasks.push(std::move(f));
  }
  m_cv.notify_one();
  return true;
}

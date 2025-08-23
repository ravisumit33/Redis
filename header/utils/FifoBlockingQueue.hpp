#pragma once

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

class FifoBlockingQueue {
public:
  class WaitToken {
  public:
    WaitToken(FifoBlockingQueue &queue);
    ~WaitToken();

    WaitToken(const WaitToken &) = delete;
    WaitToken &operator=(const WaitToken &) = delete;
    WaitToken(WaitToken &&) = delete;
    WaitToken &operator=(WaitToken &&) = delete;

    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period> &timeout) {
      std::unique_lock<std::mutex> lock(m_mutex);
      return m_cv.wait_for(lock, timeout) == std::cv_status::no_timeout;
    }

    template <typename Rep, typename Period, typename Predicate>
    bool wait_for(const std::chrono::duration<Rep, Period> &timeout,
                  Predicate predicate) {
      std::unique_lock<std::mutex> lock(m_mutex);
      return m_cv.wait_for(lock, timeout, predicate);
    }

    void wait() {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock);
    }

    template <typename Predicate> void wait(Predicate predicate) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, predicate);
    }

    void notify() { m_cv.notify_one(); }

  private:
    FifoBlockingQueue *m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    friend class FifoBlockingQueue;
  };

  std::unique_ptr<WaitToken> create_wait_token();

  void notify_one();

  void notify_all();

  std::size_t waiting_count() const;

  ~FifoBlockingQueue();

private:
  mutable std::mutex m_queue_mutex;
  std::list<WaitToken *> m_waiting_list;

  void remove_wait_token(WaitToken *token);

  friend class WaitToken;
};

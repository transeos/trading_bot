// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Subhagato Dutta
//
//*****************************************************************

#ifndef CONCURRENT_QUEUE
#define CONCURRENT_QUEUE

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class ConcurrentQueue {
 public:
  T pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    if (queue_.empty()) {
      cond_.wait(mlock);
    }

    if (queue_.empty()) throw std::runtime_error("Queue destructed :(");
    auto val = queue_.front();
    queue_.pop();
    return val;
  }

  T try_pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
    std::unique_lock<std::mutex> mlock(mutex_);
    bool timeout_occured = false;
    T val;
    if (queue_.empty()) {
      timeout_occured = (cond_.wait_for(mlock, timeout) == std::cv_status::timeout);
    }

    if (timeout_occured) {
      return val;
    } else if (queue_.empty()) {
      throw std::runtime_error("Queue destructed :(");
    } else {
      val = queue_.front();
      queue_.pop();
      return val;
    }
  }

  void push(const T& item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }

  T front() {
    std::unique_lock<std::mutex> mlock(mutex_);
    if (queue_.empty()) {
      throw std::underflow_error("buffer is empty!");
    } else {
      auto val = queue_.front();
      return val;
    }
  }

  T back() {
    std::unique_lock<std::mutex> mlock(mutex_);
    if (queue_.empty()) {
      throw std::underflow_error("buffer is empty!");
    } else {
      auto val = queue_.back();
      return val;
    }
  }

  int size() {
    std::unique_lock<std::mutex> mlock(mutex_);
    return queue_.size();
  }

  ConcurrentQueue() = default;

  ~ConcurrentQueue() {
    std::unique_lock<std::mutex> mlock(mutex_);
    cond_.notify_all();
  };

  ConcurrentQueue(const ConcurrentQueue&) = delete;             // disable copying
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;  // disable assignment

 private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  int count;
};

#endif

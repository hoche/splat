// Simple workqueue loosely adapted from stackexchange
// https://codereview.stackexchange.com/questions/60363/thread-pool-worker-implementation

#pragma once

#include <cassert>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

class WorkQueue {
  public:
    // Initialize a workqueue with a requested number of workers.
    //
    // If numWorkers is less than 0, use the number of CPU threads available on
    // the platform.
    //
    explicit WorkQueue(int numWorkers = -1);

    // Initialize a workqueue with a worklist and a requested number of workers.
    //
    // We own the memory for the worklist after this.
    //
    // If numWorkers is less than 0, use the number of CPU threads available on
    // the platform.
    //
    explicit WorkQueue(std::deque<std::function<void()>> &worklist,
                       int numWorkers = -1);

    // Stop processing work right away and dispose of threads
    virtual ~WorkQueue();

    static int maxWorkers();

    // Stop processing work right away and dispose of threads
    void abort();

    // Finish all work and then dispose of threads afterwards
    void waitForCompletion();

    void submit(std::function<void()> job, bool blocking = true);

  private:
    // Thread main loop
    void doWork();
    void joinAll();

  private:
    std::deque<std::function<void()>> m_work;
    std::vector<std::thread> m_workers;
    std::mutex m_mutex;

    std::condition_variable m_signalWaiting;
    std::condition_variable m_signalWorkDone;

    std::atomic<bool> m_exit{false};
    std::atomic<bool> m_finish_work{
        true};  // override m_exit until the work is done

    void operator=(const WorkQueue &) = delete;
    WorkQueue(const WorkQueue &) = delete;
};

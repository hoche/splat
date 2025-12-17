/** @file workqueue.cpp
 *
 * Simple workqueue
 * File created by Michel Hoche-Mong (KF6ZST)
 * Derived from original project code.
 * Splat!
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "workqueue.h"

WorkQueue::WorkQueue(int numWorkers) {
    if (numWorkers < 1) {
        numWorkers = std::thread::hardware_concurrency();
    }
    if (numWorkers < 1) {
        numWorkers = 1;
    }

    while (numWorkers--) {
        m_workers.emplace_back(std::thread(&WorkQueue::doWork, this));
    }
}

// Initialize a workqueue with a worklist and a requested number of workers.
//
// We own the memory for the worklist after this.
//
// If numWorkers is less than 0, use the number of CPU threads available on the
// platform.
//
WorkQueue::WorkQueue(std::deque<std::function<void()>> &worklist,
                     int numWorkers)
    : m_work(worklist) {
    if (numWorkers < 1) {
        numWorkers = std::thread::hardware_concurrency();
    }
    if (numWorkers < 1) {
        numWorkers = 1;
    }

    while (numWorkers--) {
        m_workers.emplace_back(std::thread(&WorkQueue::doWork, this));
    }
}

// Stop processing work right away and dispose of threads
WorkQueue::~WorkQueue() { abort(); }

int WorkQueue::maxWorkers() { return std::thread::hardware_concurrency(); }

// Stop processing work right away and dispose of threads
void WorkQueue::abort() {
    m_exit = true;
    m_finish_work = false;
    m_signalWaiting.notify_all();

    joinAll();

    {
        std::lock_guard<std::mutex> lg(m_mutex);
        m_work.clear();
    }
}

// Finish all work and then dispose of threads afterwards
void WorkQueue::waitForCompletion() {
    m_exit = true;
    m_finish_work = true;
    m_signalWaiting.notify_all();

    joinAll();
}

void WorkQueue::submit(std::function<void()> job, bool blocking) {
    std::unique_lock<std::mutex> ul(m_mutex);  // constructed locked
    if (m_exit || ! m_finish_work) {
        return;
    }
    if (blocking && (m_work.size() >= m_workers.size())) {
        // wait until a worker's available
        // fprintf(stderr, "Blocking until a thread is available\n");
        m_signalWorkDone.wait(ul);
    }

    // Ok to add and return;
    // fprintf(stderr, "Adding job\n");
    m_work.emplace_back(job);
    m_signalWaiting.notify_one();
}

// Thread main loop
void WorkQueue::doWork() {
    std::unique_lock<std::mutex> ul(m_mutex);  // constructed locked

    while (! m_exit || (m_finish_work && ! m_work.empty())) {
        if (! m_work.empty()) {
            // fprintf(stderr, "    Thr[%d]: Working.\n", myId);
            std::function<void()> work(std::move(m_work.front()));
            m_work.pop_front();
            ul.unlock();
            work();
            ul.lock();
            // fprintf(stderr, "    Thr[%d]: Done.\n", myId);
            m_signalWorkDone
                .notify_one();  // This just notifies any blocking submits
        } else {
            // fprintf(stderr, "    Thr[%d]: Paused for more work.\n", myId);
            m_signalWaiting.wait(
                ul);  // Wait until either new work is added or we're cleaning up
        }
    }
}

void WorkQueue::joinAll() {
    for (auto &thread : m_workers) {
        thread.join();
    }
    m_workers.clear();
    m_work.clear();
}

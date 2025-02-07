#include "TaskScheduler.h"

bool Task::operator>(const Task& other) const {
    return executeAt > other.executeAt;
}

TaskScheduler::TaskScheduler() : stop(false) {
    worker = thread([this] { run(); });
}

void TaskScheduler::schedule(const function<void()>& task, int delay_ms) {
    unique_lock<mutex> lock(mtx);
    taskQueue.push({task, chrono::steady_clock::now() + chrono::milliseconds(delay_ms)});
    cv.notify_one();
}

void TaskScheduler::stopScheduler() {
    {
        unique_lock<mutex> lock(mtx);
        stop = true;
    }
    cv.notify_one();
    if (worker.joinable()) {
        worker.join();
    }
}

TaskScheduler::~TaskScheduler() {
    stopScheduler();
}

void TaskScheduler::run() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        if (stop && taskQueue.empty()) break;

        if (taskQueue.empty()) {
            cv.wait(lock);
        } else {
            auto now = chrono::steady_clock::now();
            if (taskQueue.top().executeAt > now) {
                cv.wait_until(lock, taskQueue.top().executeAt);
            }

            if (!taskQueue.empty() && taskQueue.top().executeAt <= chrono::steady_clock::now()) {
                auto task = taskQueue.top();
                taskQueue.pop();
                lock.unlock();
                task.func();
                lock.lock();
            }
        }
    }
}
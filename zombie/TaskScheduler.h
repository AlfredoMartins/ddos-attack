#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <curl/curl.h>

using namespace std;

struct Task {
    function<void()> func;
    chrono::steady_clock::time_point executeAt;

    bool operator>(const Task& other) const;
};

class TaskScheduler {
public:
    TaskScheduler();
    void schedule(const function<void()>& task, int delay_ms);
    void stopScheduler();
    ~TaskScheduler();

private:
    priority_queue<Task, vector<Task>, greater<Task>> taskQueue;
    mutex mtx;
    condition_variable cv;
    bool stop;
    thread worker;

    void run();
};

#endif
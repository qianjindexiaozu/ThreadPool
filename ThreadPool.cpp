#include "ThreadPool.h"
#include <cstdio>
#include <functional>
#include <mutex>
#include <thread>


ThreadPool::ThreadPool(int min, int max) : m_minThreads(min), m_maxThreads(max), 
    m_curThreads(min), m_idleThreads(min), m_stop(false) {
    // 管理者线程
    m_manager = new thread(&ThreadPool::manager, this);

    // 工作线程
    for (int i = 0; i < min; i++) {
        m_workers.emplace_back(thread(&ThreadPool::worker, this));
    }
    printf("ThreadPool创建完成\n");
}

ThreadPool::~ThreadPool() {
}

void ThreadPool::manager() {
    printf("我是管理者线程\n");
}

void ThreadPool::worker() {
    printf("我是工作线程\n");
}

void ThreadPool::addTask(function<void(void)> task) {
    {
        lock_guard<mutex> locker(m_queueMutex); 
        m_tasks.emplace(task);
    }
    m_condition.notify_one();
}

int main() {

    ThreadPool tp = ThreadPool(2, 4);
    printf("我是主函数\n");
    
    return 0;
}

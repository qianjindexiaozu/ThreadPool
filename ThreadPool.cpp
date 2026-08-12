#include "ThreadPool.h"
#include <chrono>
#include <cstdio>
#include <functional>
#include <mutex>
#include <thread>
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : m_minThreads(min), m_maxThreads(max), 
    m_curThreads(max), m_idleThreads(max), m_stop(false) {
    // 管理者线程
    m_manager = new thread(&ThreadPool::manager, this);

    // 工作线程
    for (int i = 0; i < max; i++) {
        thread t(&ThreadPool::worker, this);
        m_workers.insert(make_pair(t.get_id(), std::move(t)));
    }
    cout << "ThreadPool创建完成" << endl;
    cout << "minThreads: " << m_minThreads << endl;
    cout << "maxThreads: " << m_maxThreads << endl;
}

ThreadPool::~ThreadPool() {
    m_stop.store(true);
    m_condition.notify_all();
    for (auto& it : m_workers) {
        thread& t = it.second;
        if (t.joinable()) {
            cout << t.get_id() << "线程将要被销毁" << endl;
            t.join();
        }
    }
    if (m_manager->joinable()) {
        m_manager->join();
    }
    delete m_manager;
}

void ThreadPool::manager() {
    while(!m_stop.load()) {
        this_thread::sleep_for(chrono::seconds(1));
        int idle = m_idleThreads.load();
        int cur = m_curThreads.load();
        if(idle > cur / 2 && cur > m_minThreads) {
            // 每次退出两个线程
            m_exitThreads.store(2);
            m_condition.notify_all();
            lock_guard<mutex> lck(m_idsMutex);
            for (auto id : m_ids) {
                auto it = m_workers.find(id);
                if (it != m_workers.end() && (*it).second.joinable()) {
                    cout << "线程" << (*it).first << "被销毁" << endl;
                    (*it).second.join();
                    m_workers.erase(it);
                }
            }
            m_ids.clear();
        }
        else if(m_idleThreads == 0 && m_curThreads < m_maxThreads) {
            thread t(&ThreadPool::worker, this);
            m_workers.insert(make_pair(t.get_id(), std::move(t)));
            m_curThreads++;
            m_idleThreads++;
        }
    }
}

void ThreadPool::worker() {
    while(!m_stop.load()) {
        function<void(void)> task = nullptr;
        {
            unique_lock<mutex> locker(m_queueMutex);
            m_condition.wait(locker, [this]{
                return !m_tasks.empty() || m_stop || m_exitThreads.load() > 0;
            });
            if(m_exitThreads.load() > 0) {
                lock_guard<mutex> lck(m_idsMutex);
                cout << "线程" << this_thread::get_id() << "退出" << endl;
                m_ids.emplace_back(this_thread::get_id());
                m_exitThreads--;
                m_idleThreads--;
                m_curThreads--;
                return;
            }
            if(!m_tasks.empty()) {
                cout << "线程" << this_thread::get_id() << "取出了一个任务" << endl;
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
        }
        if(task) {
            m_idleThreads--;
            task();
            m_idleThreads++;
        }
    }
}

void ThreadPool::addTask(function<void(void)> task) {
    {
        lock_guard<mutex> locker(m_queueMutex); 
        m_tasks.emplace(task);
    }
    m_condition.notify_one();
}

void calc(int x, int y) {
    int z = x + y;
    cout << "x + y = " << z << endl;
    this_thread::sleep_for(chrono::seconds(2));
}


int calc_int(int x, int y) {
    int z = x + y;
    this_thread::sleep_for(chrono::seconds(2));
    return z;
}

int main() {

    ThreadPool tp;

    for (int i = 0; i < 10; i++) {
        auto obj = bind(calc, i, i * 2);
        tp.addTask(obj);
    }

    getchar();
    return 0;
}

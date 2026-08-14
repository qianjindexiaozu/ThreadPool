#include "ThreadPool.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <iostream>
#include <vector>

ThreadPool::ThreadPool(int min, int max) : m_minThreads(min), m_maxThreads(max), 
    m_curThreads(max), m_idleThreads(max), m_exitThreads(0), m_stop(false) {
    // 管理者线程
    m_manager = new thread(&ThreadPool::manager, this);

    // 工作线程
    for (int i = 0; i < max; i++) {
        thread t(&ThreadPool::worker, this);
        m_workers.insert(make_pair(t.get_id(), std::move(t)));
    }
    cout << "ThreadPool创建完成" << endl;
    cout << "minThreads: " << m_minThreads.load() << endl;
    cout << "maxThreads: " << m_maxThreads.load() << endl;

}

ThreadPool::~ThreadPool() {
    m_stop.store(true);
    m_condition.notify_all();
    m_manager_condition.notify_one();
    if (m_manager->joinable()) {
        m_manager->join();
    }
    for (auto& it : m_workers) {
        thread& t = it.second;
        if (t.joinable()) {
            cout << t.get_id() << "线程将要被销毁" << endl;
            t.join();
        }
    }
    delete m_manager;
}

void ThreadPool::manager() {
    while(!m_stop.load()) {
        cv_status status;
        {
            unique_lock<mutex> locker(m_queueMutex);
            status = m_manager_condition.wait_for(locker, chrono::seconds(1));
        }
        if (m_stop.load()) {
            break;
        }
        if (status == cv_status::timeout) {
            bool needNotify = false;
            if(m_idleThreads >= m_curThreads / 2 && m_curThreads > m_minThreads) {
                int exitNum = min(2, m_curThreads - m_minThreads.load());
                unique_lock<mutex> locker(m_queueMutex);
                if (m_exitThreads.load() == 0) {
                    m_exitThreads.store(exitNum);
                    needNotify = true;
                }
            }

            if (needNotify) {
                m_condition.notify_all();
            }

            vector<thread::id> ids;
            {
                lock_guard<mutex> lck(m_idsMutex);
                ids.swap(m_ids);
            }
            for (auto id : ids) {
                auto it = m_workers.find(id);
                if (it != m_workers.end()) {
                    if ((*it).second.joinable()) {
                        (*it).second.join();
                    }
                    cout << "线程" << (*it).first << "被销毁" << endl;
                    m_workers.erase(it);
                }
            }
        }
        if(m_idleThreads == 0 && m_curThreads < m_maxThreads) {
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
            if(!m_tasks.empty()) {
                cout << "线程" << this_thread::get_id() << "取出了一个任务" << endl;
                task = std::move(m_tasks.front());
                m_tasks.pop();
                m_idleThreads--;
            }
            else if(m_exitThreads.load() > 0) {
                lock_guard<mutex> lck(m_idsMutex);
                cout << "线程" << this_thread::get_id() << "退出" << endl;
                m_ids.emplace_back(this_thread::get_id());
                m_exitThreads--;
                m_idleThreads--;
                m_curThreads--;
                return;
            }

        }
        if(task) {
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

    ThreadPool pool;
    vector<future<int>> results;

    for (int i = 0; i < 5; i++) {
        results.emplace_back(pool.addTask(calc_int, i, i * 2));
    }

    this_thread::sleep_for(chrono::seconds(2));

    for (int i = 0; i < 10; i++) {
        results.emplace_back(pool.addTask(calc_int, i, i * 2));
    }

    for (auto& res : results) {
        cout << "执行结果：" << res.get() << endl;
    }

    return 0;
}

#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

using namespace std;
/*
* 构成：
* 1. 管理者线程：子线程1个
*   - 调节控制工作线程数量
* 2. 工作线程：子线程n个 
*   - 从工作队列中取任务并处理
*   - 如果工作线程为空，则被条件变量阻塞，
*   - 线程同步(互斥锁)
*   - 当前线程数量，空闲线程数量
*   - 最大、最小线程数量
* 3. 工作队列：stl -> queue
*   - 互斥锁
*   - 条件变量
* 4. 线程池开关 -> bool
*   - 如果关闭则需要销毁全部线程
*/

class ThreadPool {
public:
    
private:
    thread* m_manager;
    vector<thread> m_workers;
    atomic<int> m_curThreads;
    atomic<int> m_idleThreads;
    atomic<int> m_maxThreads;
    atomic<int> m_minThreads;
    atomic<bool> m_stop;
    queue<function<void(void)>> m_tasks;
    mutex m_queueMutex;
    condition_variable m_condition_variable;

};

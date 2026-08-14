#include <future>
#include <memory>
#include <thread>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>
#include <queue>
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
    ThreadPool(int min = 2, int max = thread::hardware_concurrency());
    ~ThreadPool();
    void addTask(function<void(void)> task);
    template<typename F, typename... Args> 
    auto addTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> { 
        using resultType = typename result_of<F(Args...)>::type; 

        // 1. 将 函数和参数绑定的可调用对象 包装成任务对象，并交由智能共享指针管理
        auto mytask = make_shared<packaged_task<resultType()>> (
            bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        // 2. 得到future
        future<resultType> res = mytask->get_future();

        // 3. 将任务加入任务队列
        m_queueMutex.lock();
        m_tasks.emplace([mytask] () {
            (*mytask)();
        });
        m_queueMutex.unlock();

        if (m_idleThreads.load() == 0) {
            m_manager_condition.notify_one();
        }

        m_condition.notify_one();

        return res;
    }
private:
    void manager();
    void worker();
private:
    thread* m_manager;
    condition_variable m_manager_condition;
    map<thread::id, thread> m_workers;
    vector<thread::id> m_ids;
    mutex m_idsMutex;
    atomic<int> m_curThreads;
    atomic<int> m_idleThreads;
    atomic<int> m_exitThreads;
    atomic<int> m_maxThreads;
    atomic<int> m_minThreads;
    atomic<bool> m_stop;
    queue<function<void(void)>> m_tasks;
    mutex m_queueMutex;
    condition_variable m_condition;

};

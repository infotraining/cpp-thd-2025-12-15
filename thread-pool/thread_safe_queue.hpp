#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
public:
    bool empty() const
    {
        std::lock_guard lk{m_queueMutex};
        return m_queue.empty();
    }

    void push(const T& item)
    {
        {
            std::lock_guard lg{m_queueMutex};
            m_queue.push(item);
        }
        m_cvQueueNotEmpty.notify_one();
    }

    void push(T&& item)
    {
        {
            std::lock_guard lg{m_queueMutex};
            m_queue.push(std::move(item));
        }
        m_cvQueueNotEmpty.notify_one();
    }

    void push(const std::vector<T>& items)
    {
        {
            std::lock_guard lk{m_queueMutex};
            for (const auto& item : items)
            {
                m_queue.push(item);
            }
        }
        m_cvQueueNotEmpty.notify_all();
    }

    void pop(T& item)
    {
        std::unique_lock ul{m_queueMutex};
        // while (m_queue.empty())
        // {
        //     m_cvQueueNotEmpty.wait(ul);
        // }
        m_cvQueueNotEmpty.wait(ul, [this] { return !m_queue.empty(); });

        item = std::move(m_queue.front());
        m_queue.pop();
    }

    bool try_pop(T& item)
    {
        std::unique_lock lk{m_queueMutex, std::try_to_lock}; // ctor unique_lock tries to acquire mutex with m.try_lock()

        if (!lk.owns_lock() || m_queue.empty())
            return false;

        item = std::move(m_queue.front());
        m_queue.pop();

        return true;
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_cvQueueNotEmpty;
};

#endif // THREAD_SAFE_QUEUE_HPP

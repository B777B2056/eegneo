#pragma once
#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include "third/concurrentqueue/concurrentqueue.h"

namespace eegneo
{
    class ThreadPool
    {
    public:
        using TaskType = std::function<void()>;

    public:
        ThreadPool(std::size_t threadNum);
        ~ThreadPool();

        void submitTask(TaskType task);

    private:
        std::atomic<bool> mIsStop_;
        std::vector<std::thread> mThreads_;
        moodycamel::ConcurrentQueue<TaskType> mTaskQueue_;

        void run();
    };
}   // namespace eegneo

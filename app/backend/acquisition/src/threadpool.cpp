#include "threadpool.h"

namespace eegneo
{
    ThreadPool::ThreadPool(std::size_t threadNum)
        : mIsStop_(false)
    {
        for (std::size_t i = 0; i < threadNum; ++i)
        {
            this->mThreads_.emplace_back(&ThreadPool::run, this);
        }
    }

    ThreadPool::~ThreadPool()
    {
        this->mIsStop_ = true;
        for (std::thread& t : this->mThreads_)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

    void ThreadPool::submitTask(ThreadPool::TaskType task)
    {
        this->mTaskQueue_.enqueue(task);
    }

    void ThreadPool::run()
    {
        while (!this->mIsStop_)
        {
            if (TaskType task; this->mTaskQueue_.try_dequeue(task))
            {
                task();
            }
        }
    }
}   // namespace eegneo

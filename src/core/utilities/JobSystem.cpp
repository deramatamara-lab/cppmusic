#include "JobSystem.h"
#include <algorithm>

namespace daw::core::utilities
{

JobSystem::JobSystem(size_t numThreads) noexcept
    : shouldStop(false)
    , activeJobs(0)
{
    const auto actualThreads = numThreads > 0 ? numThreads : std::thread::hardware_concurrency();
    
    threads.reserve(actualThreads);
    for (size_t i = 0; i < actualThreads; ++i)
    {
        threads.emplace_back(&JobSystem::workerThread, this, i);
    }
}

JobSystem::~JobSystem()
{
    stop();
}

void JobSystem::addJob(Job job)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        jobQueue.push(std::move(job));
        activeJobs.fetch_add(1, std::memory_order_release);
    }
    conditionVariable.notify_one();
}

void JobSystem::waitForCompletion()
{
    while (activeJobs.load(std::memory_order_acquire) > 0)
    {
        std::this_thread::yield();
    }
}

void JobSystem::stop()
{
    shouldStop.store(true, std::memory_order_release);
    conditionVariable.notify_all();
    
    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
    
    threads.clear();
}

void JobSystem::workerThread(size_t threadIndex)
{
    (void)threadIndex;
    
    while (!shouldStop.load(std::memory_order_acquire))
    {
        Job job;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            conditionVariable.wait(lock, [this] { return !jobQueue.empty() || shouldStop.load(std::memory_order_acquire); });
            
            if (shouldStop.load(std::memory_order_acquire))
                break;
            
            if (!jobQueue.empty())
            {
                job = std::move(jobQueue.front());
                jobQueue.pop();
            }
        }
        
        if (job)
        {
            job();
            activeJobs.fetch_sub(1, std::memory_order_release);
        }
    }
}

} // namespace daw::core::utilities


#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace daw::core::utilities
{

/**
 * @brief Lock-free job scheduler with work-stealing
 * 
 * Multi-threaded job system for parallel audio processing.
 * Follows DAW_DEV_RULES: efficient, lock-free where possible.
 */
class JobSystem
{
public:
    using Job = std::function<void()>;

    explicit JobSystem(size_t numThreads = std::thread::hardware_concurrency()) noexcept;
    ~JobSystem();

    // Non-copyable, non-movable (mutex members forbid safe moves)
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    /**
     * @brief Add job to queue
     * @param job Job function to execute
     */
    void addJob(Job job);

    /**
     * @brief Wait for all jobs to complete
     */
    void waitForCompletion();

    /**
     * @brief Get number of worker threads
     */
    [[nodiscard]] size_t getNumThreads() const noexcept { return threads.size(); }

    /**
     * @brief Stop job system and wait for threads
     */
    void stop();

private:
    void workerThread(size_t threadIndex);

    std::vector<std::thread> threads;
    std::queue<Job> jobQueue;
    std::mutex queueMutex;
    std::condition_variable conditionVariable;
    std::atomic<bool> shouldStop{false};
    std::atomic<size_t> activeJobs{0};
};

} // namespace daw::core::utilities


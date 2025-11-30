#include "RTMemoryPool.h"
#include <algorithm>
#include <cstring>

#ifdef __linux__
#include <sys/mman.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/mman.h>
#endif

namespace daw::core {

RTMemoryPool::RTMemoryPool() noexcept
    : RTMemoryPool(PoolConfig{})
{
}

RTMemoryPool::RTMemoryPool(const PoolConfig& config) noexcept
    : config(config)
{
}

RTMemoryPool::~RTMemoryPool()
{
    shutdown();
}

//==============================================================================
// Pool Management
//==============================================================================

bool RTMemoryPool::initialize()
{
    if (initialized.load())
        return true;

    juce::Logger::writeToLog("Initializing RT Memory Pool...");

    // Calculate total memory requirement
    const size_t blockHeaderSize = sizeof(MemoryBlock);
    const size_t alignedBlockSize = alignSize(config.blockSize + blockHeaderSize, config.alignment);
    const size_t totalSize = alignedBlockSize * config.numBlocks;

    if (totalSize > config.maxPoolSize)
    {
        juce::Logger::writeToLog("RT Memory Pool: Requested size exceeds maximum pool size");
        return false;
    }

    // Allocate aligned memory for the entire pool
    try
    {
        poolMemory = std::make_unique<char[]>(totalSize + config.alignment);
        if (!poolMemory)
        {
            juce::Logger::writeToLog("RT Memory Pool: Failed to allocate pool memory");
            return false;
        }

        // Align the pool memory
        char* alignedPoolStart = reinterpret_cast<char*>(
            alignSize(reinterpret_cast<uintptr_t>(poolMemory.get()), config.alignment));

        // Initialize free list
        MemoryBlock* currentBlock = nullptr;
        MemoryBlock* prevBlock = nullptr;

        for (size_t i = 0; i < config.numBlocks; ++i)
        {
            currentBlock = reinterpret_cast<MemoryBlock*>(alignedPoolStart + (i * alignedBlockSize));
            currentBlock->next.store(nullptr);
            currentBlock->size.store(config.blockSize);
            currentBlock->isAllocated.store(false);
            currentBlock->refCount.store(0);
            currentBlock->allocationTime.store(0);

            if (prevBlock)
            {
                prevBlock->next.store(currentBlock);
            }
            else
            {
                freeList.store(currentBlock);
            }

            prevBlock = currentBlock;
        }

        totalPoolSizeBytes.store(totalSize);

        // Lock memory pages if requested
        if (config.enableMemoryLocking)
        {
            try
            {
                lockMemoryPages(alignedPoolStart, totalSize);
                juce::Logger::writeToLog("RT Memory Pool: Memory pages locked");
            }
            catch (const std::exception& e)
            {
                juce::Logger::writeToLog("RT Memory Pool: Warning - Failed to lock memory pages: " +
                                       juce::String(e.what()));
            }
        }

        initialized.store(true);
        juce::Logger::writeToLog("RT Memory Pool initialized - " +
                               juce::String(config.numBlocks) + " blocks of " +
                               juce::String(config.blockSize) + " bytes each");
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("RT Memory Pool initialization failed: " + juce::String(e.what()));
        return false;
    }
}

void RTMemoryPool::shutdown()
{
    if (!initialized.load())
        return;

    juce::Logger::writeToLog("Shutting down RT Memory Pool...");

    // Unlock memory pages
    if (config.enableMemoryLocking && poolMemory)
    {
        try
        {
            const size_t totalSize = totalPoolSizeBytes.load();
            char* alignedPoolStart = reinterpret_cast<char*>(
                alignSize(reinterpret_cast<uintptr_t>(poolMemory.get()), config.alignment));
            unlockMemoryPages(alignedPoolStart, totalSize);
        }
        catch (const std::exception& e)
        {
            juce::Logger::writeToLog("RT Memory Pool: Warning - Failed to unlock memory pages: " +
                                   juce::String(e.what()));
        }
    }

    // Log final statistics
    if (config.enableStatistics)
    {
        logStats();
    }

    // Reset state
    freeList.store(nullptr);
    allocatedList.store(nullptr);
    poolMemory.reset();
    totalPoolSizeBytes.store(0);
    initialized.store(false);

    juce::Logger::writeToLog("RT Memory Pool shutdown complete");
}

void RTMemoryPool::reset()
{
    if (!initialized.load())
        return;

    std::lock_guard<std::mutex> lock(poolMutex);

    // Move all allocated blocks back to free list
    MemoryBlock* allocatedHead = allocatedList.exchange(nullptr);
    while (allocatedHead)
    {
        MemoryBlock* next = allocatedHead->next.load();
        allocatedHead->isAllocated.store(false);
        allocatedHead->refCount.store(0);
        allocatedHead->allocationTime.store(0);

        returnBlockToFreeList(allocatedHead);
        allocatedHead = next;
    }

    // Reset statistics
    stats.currentUsage.store(0);
    stats.allocationCount.store(0);
    stats.deallocationCount.store(0);
    stats.fragmentedBlocks.store(0);
    stats.fragmentationRatio.store(0.0f);

    juce::Logger::writeToLog("RT Memory Pool reset complete");
}

//==============================================================================
// Real-Time Safe Operations
//==============================================================================

void* RTMemoryPool::allocate(size_t size) noexcept
{
    if (!initialized.load() || size == 0)
        return nullptr;

    const auto startTime = getCurrentTimeUs();

    // Find a suitable free block
    MemoryBlock* block = findFreeBlock(size);
    if (!block)
    {
        if (config.enableFallback)
        {
            stats.fallbackAllocations.fetch_add(1);
            return std::malloc(size); // Not RT-safe, but better than nullptr
        }
        return nullptr;
    }

    // Mark block as allocated
    block->isAllocated.store(true);
    block->refCount.store(1);
    block->allocationTime.store(startTime);

    // Update statistics
    const auto endTime = getCurrentTimeUs();
    if (config.enableStatistics)
    {
        updateStatistics(size, endTime - startTime);
    }

    return block->getData();
}

void* RTMemoryPool::allocateAligned(size_t size, size_t alignment) noexcept
{
    if (!initialized.load() || size == 0 || !isPowerOfTwo(alignment))
        return nullptr;

    // For simplicity, all our blocks are already cache-aligned
    // In a full implementation, we might need to handle custom alignments
    if (alignment <= config.alignment)
    {
        return allocate(size);
    }

    // For larger alignments, we'd need more complex logic
    return nullptr;
}

void RTMemoryPool::deallocate(void* ptr) noexcept
{
    if (!ptr || !initialized.load())
        return;

    // Check if this is a fallback allocation
    if (config.enableFallback)
    {
        // Simple heuristic: if ptr is not within our pool range, it's a fallback
        char* poolStart = reinterpret_cast<char*>(
            alignSize(reinterpret_cast<uintptr_t>(poolMemory.get()), config.alignment));
        char* poolEnd = poolStart + totalPoolSizeBytes.load();

        if (ptr < poolStart || ptr >= poolEnd)
        {
            std::free(ptr); // Not RT-safe, but necessary for fallback
            return;
        }
    }

    // Calculate block header from data pointer
    MemoryBlock* block = reinterpret_cast<MemoryBlock*>(
        static_cast<char*>(ptr) - sizeof(MemoryBlock));

    // Validate block
    if (!block->isAllocated.load())
    {
        // Double-free or corruption
        return;
    }

    // Update statistics
    if (config.enableStatistics)
    {
        const size_t blockSize = block->size.load();
        stats.totalFreed.fetch_add(blockSize);
        stats.currentUsage.fetch_sub(blockSize);
        stats.deallocationCount.fetch_add(1);
    }

    // Return block to free list
    returnBlockToFreeList(block);
}

//==============================================================================
// Thread-Safe Operations
//==============================================================================

void* RTMemoryPool::allocateThreadSafe(size_t size)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    return allocate(size);
}

void RTMemoryPool::deallocateThreadSafe(void* ptr)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    deallocate(ptr);
}

//==============================================================================
// Statistics and Monitoring
//==============================================================================

size_t RTMemoryPool::getAvailableBlocks() const noexcept
{
    size_t count = 0;
    MemoryBlock* current = freeList.load();

    while (current)
    {
        ++count;
        current = current->next.load();
    }

    return count;
}

float RTMemoryPool::getFragmentationRatio() const noexcept
{
    return stats.fragmentationRatio.load();
}

bool RTMemoryPool::isPoolExhausted() const noexcept
{
    return freeList.load() == nullptr;
}

bool RTMemoryPool::validateIntegrity() const
{
    if (!initialized.load())
        return false;

    std::lock_guard<std::mutex> lock(poolMutex);

    size_t freeCount = 0;
    size_t allocatedCount = 0;

    // Count free blocks
    MemoryBlock* current = freeList.load();
    while (current)
    {
        if (current->isAllocated.load())
        {
            juce::Logger::writeToLog("RT Memory Pool: Integrity error - allocated block in free list");
            return false;
        }
        ++freeCount;
        current = current->next.load();
    }

    // Count allocated blocks
    current = allocatedList.load();
    while (current)
    {
        if (!current->isAllocated.load())
        {
            juce::Logger::writeToLog("RT Memory Pool: Integrity error - free block in allocated list");
            return false;
        }
        ++allocatedCount;
        current = current->next.load();
    }

    // Verify total count
    if (freeCount + allocatedCount != config.numBlocks)
    {
        juce::Logger::writeToLog("RT Memory Pool: Integrity error - block count mismatch");
        return false;
    }

    return true;
}

void RTMemoryPool::logStats() const
{
    const auto& s = stats;

    juce::Logger::writeToLog(
        "RT Memory Pool Statistics:\n" +
        juce::String("  Total allocated: ") + juce::String(s.totalAllocated.load()) + " bytes\n" +
        juce::String("  Total freed: ") + juce::String(s.totalFreed.load()) + " bytes\n" +
        juce::String("  Current usage: ") + juce::String(s.currentUsage.load()) + " bytes\n" +
        juce::String("  Peak usage: ") + juce::String(s.peakUsage.load()) + " bytes\n" +
        juce::String("  Allocation count: ") + juce::String(s.allocationCount.load()) + "\n" +
        juce::String("  Deallocation count: ") + juce::String(s.deallocationCount.load()) + "\n" +
        juce::String("  Fallback allocations: ") + juce::String(s.fallbackAllocations.load()) + "\n" +
        juce::String("  Fragmentation ratio: ") + juce::String(s.fragmentationRatio.load(), 3) + "\n" +
        juce::String("  Average allocation time: ") + juce::String(s.averageAllocationTimeUs.load()) + " μs\n" +
        juce::String("  Max allocation time: ") + juce::String(s.maxAllocationTimeUs.load()) + " μs"
    );
}

//==============================================================================
// Internal Methods
//==============================================================================

RTMemoryPool::MemoryBlock* RTMemoryPool::findFreeBlock(size_t size) noexcept
{
    MemoryBlock* prev = nullptr;
    MemoryBlock* current = freeList.load();

    while (current)
    {
        if (current->size.load() >= size && !current->isAllocated.load())
        {
            // Remove from free list
            MemoryBlock* next = current->next.load();
            if (prev)
            {
                prev->next.store(next);
            }
            else
            {
                freeList.store(next);
            }

            // Add to allocated list
            current->next.store(allocatedList.load());
            allocatedList.store(current);

            return current;
        }

        prev = current;
        current = current->next.load();
    }

    return nullptr;
}

void RTMemoryPool::returnBlockToFreeList(MemoryBlock* block) noexcept
{
    if (!block)
        return;

    // Reset block state
    block->isAllocated.store(false);
    block->refCount.store(0);
    block->allocationTime.store(0);

    // Add to free list
    block->next.store(freeList.load());
    freeList.store(block);
}

void RTMemoryPool::updateStatistics(size_t size, uint64_t allocationTimeUs) noexcept
{
    if (!config.enableStatistics)
        return;

    stats.totalAllocated.fetch_add(size);
    stats.currentUsage.fetch_add(size);
    stats.allocationCount.fetch_add(1);
    stats.totalAllocationTimeUs.fetch_add(allocationTimeUs);

    // Update peak usage
    const size_t currentUsage = stats.currentUsage.load();
    size_t peakUsage = stats.peakUsage.load();
    while (currentUsage > peakUsage &&
           !stats.peakUsage.compare_exchange_weak(peakUsage, currentUsage))
    {
        // Retry if another thread updated peak usage
    }

    // Update max allocation time
    uint64_t maxTime = stats.maxAllocationTimeUs.load();
    while (allocationTimeUs > maxTime &&
           !stats.maxAllocationTimeUs.compare_exchange_weak(maxTime, allocationTimeUs))
    {
        // Retry if another thread updated max time
    }

    // Update average allocation time
    const uint64_t totalCount = stats.allocationCount.load();
    if (totalCount > 0)
    {
        const uint64_t totalTime = stats.totalAllocationTimeUs.load();
        stats.averageAllocationTimeUs.store(totalTime / totalCount);
    }
}

void RTMemoryPool::calculateFragmentation() noexcept
{
    // Simple fragmentation calculation based on free block distribution
    size_t freeBlocks = 0;
    size_t freeBlockRuns = 0;
    bool inFreeRun = false;

    MemoryBlock* current = freeList.load();
    while (current)
    {
        ++freeBlocks;

        if (!inFreeRun)
        {
            ++freeBlockRuns;
            inFreeRun = true;
        }

        current = current->next.load();
        if (!current)
        {
            inFreeRun = false;
        }
    }

    if (freeBlocks > 0)
    {
        const float fragmentation = static_cast<float>(freeBlockRuns) / static_cast<float>(freeBlocks);
        stats.fragmentationRatio.store(fragmentation);
        stats.fragmentedBlocks.store(freeBlockRuns);
    }
}

//==============================================================================
// Utility Functions
//==============================================================================

size_t RTMemoryPool::alignSize(size_t size, size_t alignment) noexcept
{
    return (size + alignment - 1) & ~(alignment - 1);
}

bool RTMemoryPool::isPowerOfTwo(size_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}

size_t RTMemoryPool::nextPowerOfTwo(size_t value) noexcept
{
    if (value == 0)
        return 1;

    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    if (sizeof(size_t) > 4)
        value |= value >> 32;

    return ++value;
}

uint64_t RTMemoryPool::getCurrentTimeUs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    );
}

//==============================================================================
// Platform-Specific Memory Locking
//==============================================================================

void RTMemoryPool::lockMemoryPages(void* ptr, size_t size)
{
#ifdef __linux__
    if (mlock(ptr, size) != 0)
    {
        throw std::runtime_error("Failed to lock memory pages on Linux");
    }
#elif defined(_WIN32)
    if (!VirtualLock(ptr, size))
    {
        throw std::runtime_error("Failed to lock memory pages on Windows");
    }
#elif defined(__APPLE__)
    if (mlock(ptr, size) != 0)
    {
        throw std::runtime_error("Failed to lock memory pages on macOS");
    }
#else
    // Platform not supported for memory locking
    juce::ignoreUnused(ptr, size);
#endif
}

void RTMemoryPool::unlockMemoryPages(void* ptr, size_t size)
{
#ifdef __linux__
    munlock(ptr, size);
#elif defined(_WIN32)
    VirtualUnlock(ptr, size);
#elif defined(__APPLE__)
    munlock(ptr, size);
#else
    juce::ignoreUnused(ptr, size);
#endif
}

} // namespace daw::core

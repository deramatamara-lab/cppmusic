#include "LockFreeUtilities.h"
#include <algorithm>
#include <cmath>

namespace daw::core::utilities {

RTMemoryPool::RTMemoryPool(size_t totalSizeBytes)
    : poolStart_(nullptr)
    , poolSize_(totalSizeBytes)
    , freeList_(nullptr)
    , allocationCount_(0)
    , usedSize_(0)
{
    // Align pool size to page boundary for better performance
    const size_t pageSize = 4096;
    poolSize_ = ((totalSizeBytes + pageSize - 1) / pageSize) * pageSize;

    // Allocate aligned memory pool
    poolStart_ = std::aligned_alloc(pageSize, poolSize_);
    if (poolStart_ == nullptr)
    {
        throw std::bad_alloc();
    }

    initializeFreeList();
}

RTMemoryPool::~RTMemoryPool()
{
    if (poolStart_ != nullptr)
    {
        std::free(poolStart_);
    }
}

RTMemoryPool::Block RTMemoryPool::allocate(size_t size, size_t alignment)
{
    // Ensure alignment is power of 2
    if ((alignment & (alignment - 1)) != 0)
    {
        return Block{}; // Invalid alignment
    }

    // Find a suitable free block
    FreeBlock* prev = nullptr;
    FreeBlock* current = freeList_;

    while (current != nullptr)
    {
        // Calculate aligned address within this block
        uintptr_t blockStart = reinterpret_cast<uintptr_t>(current->data);
        uintptr_t alignedStart = (blockStart + alignment - 1) & ~(alignment - 1);
        size_t alignmentPadding = alignedStart - blockStart;
        size_t totalNeeded = alignmentPadding + size;

        if (totalNeeded <= current->size)
        {
            // Found suitable block
            void* userData = reinterpret_cast<void*>(alignedStart);

            // Update free block
            if (alignmentPadding == 0)
            {
                // Perfect alignment, use entire block
                if (prev != nullptr)
                {
                    prev->next = current->next;
                }
                else
                {
                    freeList_ = current->next;
                }
            }
            else
            {
                // Split block: keep alignment padding as free
                size_t remainingSize = current->size - totalNeeded;
                if (remainingSize >= sizeof(FreeBlock) + alignment)
                {
                    // Create new free block for remaining space
                    FreeBlock* newFree = reinterpret_cast<FreeBlock*>(
                        reinterpret_cast<uintptr_t>(userData) + size);

                    newFree->data = reinterpret_cast<void*>(
                        reinterpret_cast<uintptr_t>(newFree) + sizeof(FreeBlock));
                    newFree->size = remainingSize - sizeof(FreeBlock);
                    newFree->next = current->next;

                    if (prev != nullptr)
                    {
                        prev->next = newFree;
                    }
                    else
                    {
                        freeList_ = newFree;
                    }
                }
                else
                {
                    // Not enough space for new free block, use entire original block
                    if (prev != nullptr)
                    {
                        prev->next = current->next;
                    }
                    else
                    {
                        freeList_ = current->next;
                    }
                }
            }

            ++allocationCount_;
            usedSize_.fetch_add(totalNeeded, std::memory_order_relaxed);

            return Block{userData, size, alignment};
        }

        prev = current;
        current = current->next;
    }

    return Block{}; // No suitable block found
}

void RTMemoryPool::deallocate(const Block& block)
{
    if (block.data == nullptr || block.size == 0)
    {
        return;
    }

    // Create new free block
    FreeBlock* newFree = new (block.data) FreeBlock{
        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(block.data) + sizeof(FreeBlock)),
        block.size - sizeof(FreeBlock),
        nullptr
    };

    // Insert into free list (simple first-fit for now)
    newFree->next = freeList_;
    freeList_ = newFree;

    --allocationCount_;
    usedSize_.fetch_sub(block.size + sizeof(FreeBlock), std::memory_order_relaxed);
}

RTMemoryPool::Stats RTMemoryPool::getStats() const noexcept
{
    const size_t used = usedSize_.load(std::memory_order_acquire);
    const size_t free = poolSize_ - used;

    return Stats{
        poolSize_,
        used,
        free,
        allocationCount_,
        static_cast<float>(used) / static_cast<float>(poolSize_)
    };
}

void RTMemoryPool::reset()
{
    allocationCount_ = 0;
    usedSize_.store(0, std::memory_order_release);
    initializeFreeList();
}

void RTMemoryPool::initializeFreeList()
{
    // Initialize single free block covering entire pool
    freeList_ = new (poolStart_) FreeBlock{
        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(poolStart_) + sizeof(FreeBlock)),
        poolSize_ - sizeof(FreeBlock),
        nullptr
    };
}

//==============================================================================

SampleAccurateTransport::SampleAccurateTransport()
    : tempo_(128.0)
    , timeSigNumerator_(4)
    , timeSigDenominator_(4)
    , isPlaying_(false)
    , currentBeats_(0.0)
    , accumulatedSamples_(0.0)
{}

void SampleAccurateTransport::update(size_t samplesProcessed, double sampleRate) noexcept
{
    if (!isPlaying_.load(std::memory_order_acquire))
    {
        return;
    }

    accumulatedSamples_ += static_cast<double>(samplesProcessed);
    const double beatsAdvanced = accumulatedSamples_ / samplesPerBeat();

    // Update atomic position
    const double newBeats = currentBeats_.load(std::memory_order_acquire) + beatsAdvanced;
    currentBeats_.store(newBeats, std::memory_order_release);

    // Reset accumulator to maintain precision
    accumulatedSamples_ = std::fmod(accumulatedSamples_, samplesPerBeat());
}

void SampleAccurateTransport::setTempo(double bpm) noexcept
{
    tempo_.store(bpm, std::memory_order_release);
}

void SampleAccurateTransport::setTimeSignature(int numerator, int denominator) noexcept
{
    timeSigNumerator_.store(numerator, std::memory_order_release);
    timeSigDenominator_.store(denominator, std::memory_order_release);
}

void SampleAccurateTransport::setPositionInBeats(double beats) noexcept
{
    currentBeats_.store(beats, std::memory_order_release);
    accumulatedSamples_ = 0.0; // Reset accumulator when position jumps
}

void SampleAccurateTransport::start() noexcept
{
    isPlaying_.store(true, std::memory_order_release);
}

void SampleAccurateTransport::stop() noexcept
{
    isPlaying_.store(false, std::memory_order_release);
}

bool SampleAccurateTransport::isPlaying() const noexcept
{
    return isPlaying_.load(std::memory_order_acquire);
}

SampleAccurateTransport::Position SampleAccurateTransport::getCurrentPosition() const noexcept
{
    const double beats = currentBeats_.load(std::memory_order_acquire);
    const double tempo = tempo_.load(std::memory_order_acquire);
    const int numerator = timeSigNumerator_.load(std::memory_order_acquire);
    const int denominator = timeSigDenominator_.load(std::memory_order_acquire);

    const double samples = beats * samplesPerBeat();
    const double seconds = beats * 60.0 / tempo;

    return Position{
        beats,
        samples,
        seconds,
        calculateBar(beats),
        calculateBeatInBar(beats),
        calculateSixteenthInBeat(beats)
    };
}

double SampleAccurateTransport::getTempo() const noexcept
{
    return tempo_.load(std::memory_order_acquire);
}

std::pair<int, int> SampleAccurateTransport::getTimeSignature() const noexcept
{
    return {
        timeSigNumerator_.load(std::memory_order_acquire),
        timeSigDenominator_.load(std::memory_order_acquire)
    };
}

double SampleAccurateTransport::samplesPerBeat() const noexcept
{
    const double tempo = tempo_.load(std::memory_order_relaxed);
    const int denominator = timeSigDenominator_.load(std::memory_order_relaxed);

    // BPM gives quarter notes per minute, denominator gives note value
    // For 4/4 time, denominator=4 means quarter notes, so samples per beat = samples per quarter note
    // For 6/8 time, denominator=8 means eighth notes, so samples per beat = samples per eighth note
    const double beatsPerSecond = tempo / 60.0;
    const double noteValue = 4.0 / static_cast<double>(denominator); // Convert to quarter note equivalent

    return 44100.0 / (beatsPerSecond * noteValue);
}

int SampleAccurateTransport::calculateBar(double beats) const noexcept
{
    const int numerator = timeSigNumerator_.load(std::memory_order_relaxed);
    return static_cast<int>(std::floor(beats / numerator)) + 1; // 1-based
}

int SampleAccurateTransport::calculateBeatInBar(double beats) const noexcept
{
    const int numerator = timeSigNumerator_.load(std::memory_order_relaxed);
    const double beatsInBar = std::fmod(beats, numerator);
    return static_cast<int>(std::floor(beatsInBar)) + 1; // 1-based
}

int SampleAccurateTransport::calculateSixteenthInBeat(double beats) const noexcept
{
    const double fractionalBeat = std::fmod(beats, 1.0);
    return static_cast<int>(std::floor(fractionalBeat * 4.0)) + 1; // 1-based, 16ths
}

} // namespace daw::core::utilities

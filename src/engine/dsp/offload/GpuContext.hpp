/**
 * @file GpuContext.hpp
 * @brief GPU context abstraction for DSP offloading (Vulkan stub)
 *
 * This file provides a conditional GPU context that can be enabled via
 * ENABLE_GPU compile flag. When disabled, all operations gracefully
 * fall back to no-ops.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::dsp::offload {

/**
 * @brief GPU buffer handle for data transfer
 */
struct GpuBufferHandle {
    uint64_t id{0};
    size_t size{0};
    bool valid{false};
};

/**
 * @brief GPU device information
 */
struct GpuDeviceInfo {
    std::string name;
    std::string vendorId;
    uint64_t memoryBytes{0};
    uint32_t computeUnits{0};
    bool supportsFloat64{false};
    bool supportsAsyncCompute{false};
};

/**
 * @brief Result codes for GPU operations
 */
enum class GpuResult {
    Success,
    NotAvailable,
    OutOfMemory,
    DeviceLost,
    InvalidHandle,
    Timeout,
    Disabled  // Feature not compiled in
};

/**
 * @brief GPU context for DSP offloading
 *
 * This class provides a Vulkan-based GPU context stub for offloading
 * compute-intensive DSP operations. When ENABLE_GPU is not defined,
 * all operations return GpuResult::Disabled.
 *
 * Thread safety: The context is not thread-safe. Each thread should
 * use its own context or synchronize externally.
 */
class GpuContext {
public:
    GpuContext();
    ~GpuContext();

    // Non-copyable, moveable
    GpuContext(const GpuContext&) = delete;
    GpuContext& operator=(const GpuContext&) = delete;
    GpuContext(GpuContext&&) noexcept;
    GpuContext& operator=(GpuContext&&) noexcept;

    /**
     * @brief Initialize the GPU context
     * @param preferredDeviceIndex Optional device index to use
     * @return Result code indicating success or failure reason
     */
    GpuResult initialize(std::optional<uint32_t> preferredDeviceIndex = std::nullopt);

    /**
     * @brief Shutdown the GPU context and release resources
     */
    void shutdown();

    /**
     * @brief Check if GPU is available and initialized
     */
    [[nodiscard]] bool isAvailable() const;

    /**
     * @brief Get list of available GPU devices
     */
    [[nodiscard]] std::vector<GpuDeviceInfo> enumerateDevices() const;

    /**
     * @brief Get information about the active device
     */
    [[nodiscard]] std::optional<GpuDeviceInfo> getActiveDevice() const;

    /**
     * @brief Allocate a buffer on the GPU
     * @param sizeBytes Size of the buffer in bytes
     * @return Buffer handle (check .valid for success)
     */
    [[nodiscard]] GpuBufferHandle allocateBuffer(size_t sizeBytes);

    /**
     * @brief Free a previously allocated buffer
     * @param handle Buffer handle to free
     */
    void freeBuffer(GpuBufferHandle handle);

    /**
     * @brief Upload data to a GPU buffer
     * @param handle Target buffer handle
     * @param data Source data pointer
     * @param sizeBytes Size of data to upload
     * @param offsetBytes Offset in the buffer
     * @return Result code
     */
    GpuResult uploadData(GpuBufferHandle handle, const void* data, 
                         size_t sizeBytes, size_t offsetBytes = 0);

    /**
     * @brief Download data from a GPU buffer
     * @param handle Source buffer handle
     * @param data Destination data pointer
     * @param sizeBytes Size of data to download
     * @param offsetBytes Offset in the buffer
     * @return Result code
     */
    GpuResult downloadData(GpuBufferHandle handle, void* data,
                           size_t sizeBytes, size_t offsetBytes = 0);

    /**
     * @brief Submit a compute dispatch (stub)
     * @param pipelineId Pipeline identifier
     * @param workgroupsX Number of workgroups in X
     * @param workgroupsY Number of workgroups in Y
     * @param workgroupsZ Number of workgroups in Z
     * @return Result code
     */
    GpuResult dispatch(uint32_t pipelineId, uint32_t workgroupsX,
                       uint32_t workgroupsY = 1, uint32_t workgroupsZ = 1);

    /**
     * @brief Wait for all pending operations to complete
     * @param timeoutMs Maximum wait time in milliseconds
     * @return Result code (Timeout if not completed in time)
     */
    GpuResult waitIdle(uint32_t timeoutMs = 1000);

    /**
     * @brief Get estimated latency for a given buffer size transfer
     * @param sizeBytes Buffer size
     * @return Estimated latency in microseconds
     */
    [[nodiscard]] uint64_t estimateTransferLatencyUs(size_t sizeBytes) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppmusic::dsp::offload

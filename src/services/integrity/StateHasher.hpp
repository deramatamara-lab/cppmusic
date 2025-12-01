#pragma once
/**
 * @file StateHasher.hpp
 * @brief State hashing for integrity verification.
 */

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace cppmusic::services::integrity {

/**
 * @brief 256-bit hash value.
 */
struct Hash256 {
    std::array<std::uint8_t, 32> bytes{};
    
    bool operator==(const Hash256& other) const noexcept {
        return bytes == other.bytes;
    }
    
    bool operator!=(const Hash256& other) const noexcept {
        return !(*this == other);
    }
    
    /**
     * @brief Convert to hexadecimal string.
     */
    [[nodiscard]] std::string toHex() const;
    
    /**
     * @brief Parse from hexadecimal string.
     */
    static Hash256 fromHex(const std::string& hex);
    
    /**
     * @brief Get a zero hash.
     */
    static Hash256 zero() { return Hash256{}; }
};

/**
 * @brief State hashing for integrity verification.
 * 
 * Uses a fast hash function (placeholder for BLAKE3 integration).
 * Provides incremental hashing for efficient updates.
 */
class StateHasher {
public:
    StateHasher();
    ~StateHasher();
    
    // Non-copyable but movable
    StateHasher(const StateHasher&) = delete;
    StateHasher& operator=(const StateHasher&) = delete;
    StateHasher(StateHasher&&) noexcept;
    StateHasher& operator=(StateHasher&&) noexcept;
    
    // =========================================================================
    // One-Shot Hashing
    // =========================================================================
    
    /**
     * @brief Hash a byte array.
     */
    [[nodiscard]] static Hash256 hash(const std::uint8_t* data, std::size_t size);
    
    /**
     * @brief Hash a vector of bytes.
     */
    [[nodiscard]] static Hash256 hash(const std::vector<std::uint8_t>& data);
    
    /**
     * @brief Hash a string.
     */
    [[nodiscard]] static Hash256 hash(const std::string& str);
    
    // =========================================================================
    // Incremental Hashing
    // =========================================================================
    
    /**
     * @brief Reset the hasher state.
     */
    void reset();
    
    /**
     * @brief Update the hash with additional data.
     */
    void update(const std::uint8_t* data, std::size_t size);
    
    /**
     * @brief Update the hash with a vector of bytes.
     */
    void update(const std::vector<std::uint8_t>& data);
    
    /**
     * @brief Update the hash with a string.
     */
    void update(const std::string& str);
    
    /**
     * @brief Finalize and get the hash.
     */
    [[nodiscard]] Hash256 finalize();
    
    /**
     * @brief Get the current hash without finalizing.
     * 
     * Allows continuing to add data after getting intermediate hash.
     */
    [[nodiscard]] Hash256 getCurrentHash() const;
    
    // =========================================================================
    // Hash Chain Operations
    // =========================================================================
    
    /**
     * @brief Compute chained hash: H(prev || data).
     */
    [[nodiscard]] static Hash256 chainHash(const Hash256& prev,
                                           const std::vector<std::uint8_t>& data);
    
    /**
     * @brief Verify a hash chain.
     * @param hashes Sequence of hashes.
     * @param deltas Sequence of data that produced each transition.
     * @return true if chain is valid.
     */
    [[nodiscard]] static bool verifyChain(const std::vector<Hash256>& hashes,
                                          const std::vector<std::vector<std::uint8_t>>& deltas);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * @brief Combine two hashes.
 */
[[nodiscard]] Hash256 combineHashes(const Hash256& a, const Hash256& b);

} // namespace cppmusic::services::integrity

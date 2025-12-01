/**
 * @file StateHasher.cpp
 * @brief Implementation of state hashing (placeholder for BLAKE3).
 */

#include "StateHasher.hpp"
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace cppmusic::services::integrity {

// =============================================================================
// Hash256 Implementation
// =============================================================================

std::string Hash256::toHex() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

Hash256 Hash256::fromHex(const std::string& hex) {
    Hash256 hash;
    
    if (hex.size() != 64) {
        return hash;  // Invalid length
    }
    
    // Validate hex characters before parsing
    for (char c : hex) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return hash;  // Invalid character
        }
    }
    
    for (std::size_t i = 0; i < 32; ++i) {
        std::string byteStr = hex.substr(i * 2, 2);
        // std::stoul is safe here since we validated input above
        hash.bytes[i] = static_cast<std::uint8_t>(std::stoul(byteStr, nullptr, 16));
    }
    
    return hash;
}

// =============================================================================
// StateHasher Implementation (Placeholder using simple hash)
// =============================================================================

// Simple FNV-1a based hash as placeholder for BLAKE3
namespace {

constexpr std::uint64_t kFNV64Offset = 14695981039346656037ULL;
constexpr std::uint64_t kFNV64Prime = 1099511628211ULL;

std::uint64_t fnv1aHash(const std::uint8_t* data, std::size_t size, std::uint64_t hash = kFNV64Offset) {
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<std::uint64_t>(data[i]);
        hash *= kFNV64Prime;
    }
    return hash;
}

Hash256 hashToHash256(std::uint64_t h1, std::uint64_t h2, std::uint64_t h3, std::uint64_t h4) {
    Hash256 result;
    std::memcpy(result.bytes.data(), &h1, 8);
    std::memcpy(result.bytes.data() + 8, &h2, 8);
    std::memcpy(result.bytes.data() + 16, &h3, 8);
    std::memcpy(result.bytes.data() + 24, &h4, 8);
    return result;
}

} // anonymous namespace

struct StateHasher::Impl {
    std::uint64_t state[4] = {kFNV64Offset, kFNV64Offset + 1, kFNV64Offset + 2, kFNV64Offset + 3};
    std::size_t totalSize = 0;
    
    void reset() {
        state[0] = kFNV64Offset;
        state[1] = kFNV64Offset + 1;
        state[2] = kFNV64Offset + 2;
        state[3] = kFNV64Offset + 3;
        totalSize = 0;
    }
    
    void update(const std::uint8_t* data, std::size_t size) {
        // Update all four hash lanes for better distribution
        state[0] = fnv1aHash(data, size, state[0]);
        state[1] = fnv1aHash(data, size, state[1] ^ size);
        state[2] = fnv1aHash(data, size, state[2] ^ (size * 2));
        state[3] = fnv1aHash(data, size, state[3] ^ (size * 3));
        totalSize += size;
    }
    
    Hash256 finalize() const {
        // Mix final state
        std::uint64_t h1 = state[0] ^ (state[1] >> 17);
        std::uint64_t h2 = state[1] ^ (state[2] << 13);
        std::uint64_t h3 = state[2] ^ (state[3] >> 11);
        std::uint64_t h4 = state[3] ^ (state[0] << 7) ^ totalSize;
        
        return hashToHash256(h1, h2, h3, h4);
    }
};

StateHasher::StateHasher()
    : pImpl_(std::make_unique<Impl>()) {
}

StateHasher::~StateHasher() = default;

StateHasher::StateHasher(StateHasher&&) noexcept = default;
StateHasher& StateHasher::operator=(StateHasher&&) noexcept = default;

Hash256 StateHasher::hash(const std::uint8_t* data, std::size_t size) {
    StateHasher hasher;
    hasher.update(data, size);
    return hasher.finalize();
}

Hash256 StateHasher::hash(const std::vector<std::uint8_t>& data) {
    return hash(data.data(), data.size());
}

Hash256 StateHasher::hash(const std::string& str) {
    return hash(reinterpret_cast<const std::uint8_t*>(str.data()), str.size());
}

void StateHasher::reset() {
    pImpl_->reset();
}

void StateHasher::update(const std::uint8_t* data, std::size_t size) {
    pImpl_->update(data, size);
}

void StateHasher::update(const std::vector<std::uint8_t>& data) {
    update(data.data(), data.size());
}

void StateHasher::update(const std::string& str) {
    update(reinterpret_cast<const std::uint8_t*>(str.data()), str.size());
}

Hash256 StateHasher::finalize() {
    return pImpl_->finalize();
}

Hash256 StateHasher::getCurrentHash() const {
    return pImpl_->finalize();
}

Hash256 StateHasher::chainHash(const Hash256& prev, const std::vector<std::uint8_t>& data) {
    StateHasher hasher;
    hasher.update(prev.bytes.data(), prev.bytes.size());
    hasher.update(data);
    return hasher.finalize();
}

bool StateHasher::verifyChain(const std::vector<Hash256>& hashes,
                              const std::vector<std::vector<std::uint8_t>>& deltas) {
    if (hashes.empty()) return true;
    if (hashes.size() != deltas.size() + 1) return false;
    
    for (std::size_t i = 0; i < deltas.size(); ++i) {
        Hash256 computed = chainHash(hashes[i], deltas[i]);
        if (computed != hashes[i + 1]) {
            return false;
        }
    }
    
    return true;
}

Hash256 combineHashes(const Hash256& a, const Hash256& b) {
    StateHasher hasher;
    hasher.update(a.bytes.data(), a.bytes.size());
    hasher.update(b.bytes.data(), b.bytes.size());
    return hasher.finalize();
}

} // namespace cppmusic::services::integrity

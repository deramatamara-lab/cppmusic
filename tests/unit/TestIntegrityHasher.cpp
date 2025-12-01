/**
 * @file TestIntegrityHasher.cpp
 * @brief Unit tests for state hashing and integrity verification.
 */

#include "services/integrity/StateHasher.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

using namespace cppmusic::services::integrity;

namespace {

void testBasicHashing() {
    std::cout << "  Testing basic hashing..." << std::endl;
    
    std::string data = "Hello, World!";
    Hash256 hash = StateHasher::hash(data);
    
    // Hash should be non-zero - use std::all_of for clearer intent
    bool allZero = std::all_of(std::begin(hash.bytes), std::end(hash.bytes),
                               [](uint8_t byte) { return byte == 0; });
    assert(!allZero);
    
    std::cout << "  Basic hashing passed." << std::endl;
}

void testHashConsistency() {
    std::cout << "  Testing hash consistency..." << std::endl;
    
    std::string data = "Test data for hashing";
    
    Hash256 hash1 = StateHasher::hash(data);
    Hash256 hash2 = StateHasher::hash(data);
    
    // Same input should produce same hash
    assert(hash1 == hash2);
    
    std::cout << "  Hash consistency passed." << std::endl;
}

void testHashChangesWithInput() {
    std::cout << "  Testing hash changes with input..." << std::endl;
    
    std::string data1 = "Data version 1";
    std::string data2 = "Data version 2";
    
    Hash256 hash1 = StateHasher::hash(data1);
    Hash256 hash2 = StateHasher::hash(data2);
    
    // Different input should produce different hash
    assert(hash1 != hash2);
    
    std::cout << "  Hash changes with input passed." << std::endl;
}

void testIncrementalHashing() {
    std::cout << "  Testing incremental hashing..." << std::endl;
    
    StateHasher hasher1;
    hasher1.update("Hello, ");
    hasher1.update("World!");
    Hash256 incrementalHash = hasher1.finalize();
    
    // One-shot hash of combined data
    Hash256 oneShotHash = StateHasher::hash("Hello, World!");
    
    // Both methods should produce same result
    assert(incrementalHash == oneShotHash);
    
    std::cout << "  Incremental hashing passed." << std::endl;
}

void testHexConversion() {
    std::cout << "  Testing hex conversion..." << std::endl;
    
    Hash256 original = StateHasher::hash("Test");
    
    std::string hexString = original.toHex();
    
    // Hex string should be 64 characters (32 bytes * 2)
    assert(hexString.length() == 64);
    
    // Parse back
    Hash256 parsed = Hash256::fromHex(hexString);
    assert(original == parsed);
    
    std::cout << "  Hex conversion passed." << std::endl;
}

void testHashChaining() {
    std::cout << "  Testing hash chaining..." << std::endl;
    
    Hash256 initial = StateHasher::hash("Initial state");
    
    std::vector<std::uint8_t> delta1 = {'c', 'h', 'a', 'n', 'g', 'e', '1'};
    std::vector<std::uint8_t> delta2 = {'c', 'h', 'a', 'n', 'g', 'e', '2'};
    
    Hash256 hash1 = StateHasher::chainHash(initial, delta1);
    Hash256 hash2 = StateHasher::chainHash(hash1, delta2);
    
    // Chain should produce different hashes
    assert(hash1 != initial);
    assert(hash2 != hash1);
    
    // Same chain should be reproducible
    Hash256 hash1Again = StateHasher::chainHash(initial, delta1);
    Hash256 hash2Again = StateHasher::chainHash(hash1Again, delta2);
    
    assert(hash1 == hash1Again);
    assert(hash2 == hash2Again);
    
    std::cout << "  Hash chaining passed." << std::endl;
}

void testChainVerification() {
    std::cout << "  Testing chain verification..." << std::endl;
    
    Hash256 h0 = StateHasher::hash("Initial");
    
    std::vector<std::uint8_t> d1 = {'d', '1'};
    std::vector<std::uint8_t> d2 = {'d', '2'};
    
    Hash256 h1 = StateHasher::chainHash(h0, d1);
    Hash256 h2 = StateHasher::chainHash(h1, d2);
    
    std::vector<Hash256> hashes = {h0, h1, h2};
    std::vector<std::vector<std::uint8_t>> deltas = {d1, d2};
    
    // Valid chain should verify
    assert(StateHasher::verifyChain(hashes, deltas));
    
    // Corrupted chain should fail
    hashes[1].bytes[0] ^= 0xFF;  // Flip some bits
    assert(!StateHasher::verifyChain(hashes, deltas));
    
    std::cout << "  Chain verification passed." << std::endl;
}

void testHashCombine() {
    std::cout << "  Testing hash combine..." << std::endl;
    
    Hash256 a = StateHasher::hash("Hash A");
    Hash256 b = StateHasher::hash("Hash B");
    
    Hash256 combined = combineHashes(a, b);
    
    // Combined should be different from both inputs
    assert(combined != a);
    assert(combined != b);
    
    // Combining in same order should be consistent
    Hash256 combined2 = combineHashes(a, b);
    assert(combined == combined2);
    
    // Order matters (not commutative)
    Hash256 reversed = combineHashes(b, a);
    assert(combined != reversed);
    
    std::cout << "  Hash combine passed." << std::endl;
}

void testZeroHash() {
    std::cout << "  Testing zero hash..." << std::endl;
    
    Hash256 zero = Hash256::zero();
    
    bool allZero = true;
    for (const auto& byte : zero.bytes) {
        if (byte != 0) {
            allZero = false;
            break;
        }
    }
    assert(allZero);
    
    std::cout << "  Zero hash passed." << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Integrity Hasher Tests..." << std::endl;
    
    testBasicHashing();
    testHashConsistency();
    testHashChangesWithInput();
    testIncrementalHashing();
    testHexConversion();
    testHashChaining();
    testChainVerification();
    testHashCombine();
    testZeroHash();
    
    std::cout << "All Integrity Hasher Tests PASSED!" << std::endl;
    return 0;
}

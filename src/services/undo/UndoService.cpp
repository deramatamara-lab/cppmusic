/**
 * @file UndoService.cpp
 * @brief Implementation of the undo service.
 */

#include "UndoService.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>

namespace cppmusic::services::undo {

// =============================================================================
// StateDelta Implementation
// =============================================================================

std::size_t StateDelta::compressedSize() const noexcept {
    std::size_t size = path.size() + oldValue.size() + newValue.size() + 16;
    for (const auto& child : children) {
        size += child.compressedSize();
    }
    return size;
}

std::vector<std::uint8_t> StateDelta::serialize() const {
    std::vector<std::uint8_t> data;
    
    // Write type
    data.push_back(static_cast<std::uint8_t>(type));
    
    // Write path
    std::uint32_t pathLen = static_cast<std::uint32_t>(path.size());
    data.resize(data.size() + sizeof(pathLen));
    std::memcpy(data.data() + data.size() - sizeof(pathLen), &pathLen, sizeof(pathLen));
    data.insert(data.end(), path.begin(), path.end());
    
    // Write old value
    std::uint32_t oldLen = static_cast<std::uint32_t>(oldValue.size());
    data.resize(data.size() + sizeof(oldLen));
    std::memcpy(data.data() + data.size() - sizeof(oldLen), &oldLen, sizeof(oldLen));
    data.insert(data.end(), oldValue.begin(), oldValue.end());
    
    // Write new value
    std::uint32_t newLen = static_cast<std::uint32_t>(newValue.size());
    data.resize(data.size() + sizeof(newLen));
    std::memcpy(data.data() + data.size() - sizeof(newLen), &newLen, sizeof(newLen));
    data.insert(data.end(), newValue.begin(), newValue.end());
    
    // Write children count and serialize children
    std::uint32_t childCount = static_cast<std::uint32_t>(children.size());
    data.resize(data.size() + sizeof(childCount));
    std::memcpy(data.data() + data.size() - sizeof(childCount), &childCount, sizeof(childCount));
    
    for (const auto& child : children) {
        auto childData = child.serialize();
        data.insert(data.end(), childData.begin(), childData.end());
    }
    
    return data;
}

StateDelta StateDelta::deserialize(const std::vector<std::uint8_t>& data) {
    StateDelta delta;
    
    if (data.empty()) return delta;
    
    std::size_t offset = 0;
    
    // Read type
    delta.type = static_cast<Type>(data[offset++]);
    
    // Read path
    if (offset + sizeof(std::uint32_t) > data.size()) return delta;
    std::uint32_t pathLen = 0;
    std::memcpy(&pathLen, data.data() + offset, sizeof(pathLen));
    offset += sizeof(pathLen);
    
    if (offset + pathLen > data.size()) return delta;
    delta.path.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                      data.begin() + static_cast<std::ptrdiff_t>(offset + pathLen));
    offset += pathLen;
    
    // Read old value
    if (offset + sizeof(std::uint32_t) > data.size()) return delta;
    std::uint32_t oldLen = 0;
    std::memcpy(&oldLen, data.data() + offset, sizeof(oldLen));
    offset += sizeof(oldLen);
    
    if (offset + oldLen > data.size()) return delta;
    delta.oldValue.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                          data.begin() + static_cast<std::ptrdiff_t>(offset + oldLen));
    offset += oldLen;
    
    // Read new value
    if (offset + sizeof(std::uint32_t) > data.size()) return delta;
    std::uint32_t newLen = 0;
    std::memcpy(&newLen, data.data() + offset, sizeof(newLen));
    offset += sizeof(newLen);
    
    if (offset + newLen > data.size()) return delta;
    delta.newValue.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                          data.begin() + static_cast<std::ptrdiff_t>(offset + newLen));
    
    return delta;
}

// =============================================================================
// Command Default Implementation
// =============================================================================

bool Command::canMergeWith(const Command& /*other*/) const {
    return false;
}

bool Command::mergeWith(Command& /*other*/) {
    return false;
}

// =============================================================================
// UndoService Implementation
// =============================================================================

struct UndoService::Impl {
    std::vector<UndoEntry> history;
    std::size_t currentPosition = 0;
    std::size_t maxHistorySize = 1000;
    
    bool inBatch = false;
    std::size_t currentBatchId = 0;
    std::string batchDescription;
    
    std::vector<UndoListener*> listeners;
    std::mutex mutex;
    
    std::function<std::uint64_t()> stateHashProvider;
    
    std::uint64_t getStateHash() const {
        if (stateHashProvider) {
            return stateHashProvider();
        }
        return 0;
    }
};

UndoService::UndoService()
    : pImpl_(std::make_unique<Impl>()) {
}

UndoService::~UndoService() = default;

void UndoService::execute(std::unique_ptr<Command> command) {
    if (!command) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    // Execute the command
    command->execute();
    
    // If we're not at the end of history, truncate redo stack
    if (pImpl_->currentPosition < pImpl_->history.size()) {
        pImpl_->history.erase(
            pImpl_->history.begin() + static_cast<std::ptrdiff_t>(pImpl_->currentPosition),
            pImpl_->history.end());
    }
    
    // Create undo entry
    UndoEntry entry;
    entry.command = std::move(command);
    entry.timestamp = std::chrono::system_clock::now();
    entry.stateHash = pImpl_->getStateHash();
    entry.batchId = pImpl_->inBatch ? pImpl_->currentBatchId : 0;
    
    // Try to merge with previous command
    if (!pImpl_->history.empty() && !pImpl_->inBatch) {
        auto& lastEntry = pImpl_->history.back();
        if (lastEntry.command && entry.command && 
            lastEntry.command->canMergeWith(*entry.command)) {
            if (lastEntry.command->mergeWith(*entry.command)) {
                lastEntry.stateHash = entry.stateHash;
                
                // Notify listeners of the merged command
                for (auto* listener : pImpl_->listeners) {
                    if (listener) {
                        listener->onCommandExecuted(*lastEntry.command);
                    }
                }
                return;
            }
        }
    }
    
    pImpl_->history.push_back(std::move(entry));
    pImpl_->currentPosition = pImpl_->history.size();
    
    // Trim history if needed
    if (pImpl_->history.size() > pImpl_->maxHistorySize) {
        pImpl_->history.erase(pImpl_->history.begin());
        --pImpl_->currentPosition;
    }
    
    // Notify listeners
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onCommandExecuted(*pImpl_->history.back().command);
        }
    }
}

void UndoService::beginBatch(const std::string& description) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->inBatch = true;
    pImpl_->currentBatchId++;
    pImpl_->batchDescription = description;
}

void UndoService::endBatch() {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->inBatch = false;
}

bool UndoService::isInBatch() const noexcept {
    return pImpl_->inBatch;
}

bool UndoService::canUndo() const noexcept {
    return pImpl_->currentPosition > 0;
}

bool UndoService::canRedo() const noexcept {
    return pImpl_->currentPosition < pImpl_->history.size();
}

void UndoService::undo() {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    if (pImpl_->currentPosition == 0) return;
    
    // Get the batch ID of the command to undo
    std::size_t batchId = pImpl_->history[pImpl_->currentPosition - 1].batchId;
    
    // Undo all commands in the batch (or just one if not batched)
    while (pImpl_->currentPosition > 0) {
        auto& entry = pImpl_->history[pImpl_->currentPosition - 1];
        
        // Stop if we've left the batch
        if (batchId != 0 && entry.batchId != batchId) break;
        if (batchId == 0 && entry.batchId != 0) {
            // Non-batched command, just undo this one
        }
        
        entry.command->undo();
        --pImpl_->currentPosition;
        
        // Notify listeners
        for (auto* listener : pImpl_->listeners) {
            if (listener) {
                listener->onUndo(*entry.command);
            }
        }
        
        // If not in a batch, stop after one command
        if (batchId == 0) break;
    }
}

void UndoService::redo() {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    if (pImpl_->currentPosition >= pImpl_->history.size()) return;
    
    // Get the batch ID of the command to redo
    std::size_t batchId = pImpl_->history[pImpl_->currentPosition].batchId;
    
    // Redo all commands in the batch (or just one if not batched)
    while (pImpl_->currentPosition < pImpl_->history.size()) {
        auto& entry = pImpl_->history[pImpl_->currentPosition];
        
        // Stop if we've left the batch
        if (batchId != 0 && entry.batchId != batchId) break;
        
        entry.command->execute();
        ++pImpl_->currentPosition;
        
        // Notify listeners
        for (auto* listener : pImpl_->listeners) {
            if (listener) {
                listener->onRedo(*entry.command);
            }
        }
        
        // If not in a batch, stop after one command
        if (batchId == 0) break;
    }
}

std::optional<std::string> UndoService::getUndoDescription() const {
    if (pImpl_->currentPosition == 0) return std::nullopt;
    return pImpl_->history[pImpl_->currentPosition - 1].command->getDescription();
}

std::optional<std::string> UndoService::getRedoDescription() const {
    if (pImpl_->currentPosition >= pImpl_->history.size()) return std::nullopt;
    return pImpl_->history[pImpl_->currentPosition].command->getDescription();
}

std::size_t UndoService::getHistorySize() const noexcept {
    return pImpl_->history.size();
}

std::size_t UndoService::getCurrentPosition() const noexcept {
    return pImpl_->currentPosition;
}

void UndoService::jumpToPosition(std::size_t position) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    if (position > pImpl_->history.size()) {
        position = pImpl_->history.size();
    }
    
    // Undo or redo to reach target position
    while (pImpl_->currentPosition > position) {
        auto& entry = pImpl_->history[pImpl_->currentPosition - 1];
        entry.command->undo();
        --pImpl_->currentPosition;
    }
    
    while (pImpl_->currentPosition < position) {
        auto& entry = pImpl_->history[pImpl_->currentPosition];
        entry.command->execute();
        ++pImpl_->currentPosition;
    }
}

std::vector<std::string> UndoService::getHistoryDescriptions() const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    std::vector<std::string> descriptions;
    descriptions.reserve(pImpl_->history.size());
    
    for (const auto& entry : pImpl_->history) {
        descriptions.push_back(entry.command->getDescription());
    }
    
    return descriptions;
}

std::vector<std::uint8_t> UndoService::captureSnapshot() const {
    // Placeholder: Would serialize full project state
    return {};
}

void UndoService::restoreSnapshot(const std::vector<std::uint8_t>& /*snapshot*/) {
    // Placeholder: Would deserialize and restore project state
}

bool UndoService::verifyIntegrity() const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    // Placeholder: Would verify hash chain integrity
    // For now, just return true
    return true;
}

void UndoService::clear() {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    pImpl_->history.clear();
    pImpl_->currentPosition = 0;
    
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onHistoryCleared();
        }
    }
}

void UndoService::setMaxHistorySize(std::size_t size) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->maxHistorySize = size;
}

std::size_t UndoService::getMaxHistorySize() const noexcept {
    return pImpl_->maxHistorySize;
}

void UndoService::addListener(UndoListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    auto it = std::find(pImpl_->listeners.begin(), pImpl_->listeners.end(), listener);
    if (it == pImpl_->listeners.end()) {
        pImpl_->listeners.push_back(listener);
    }
}

void UndoService::removeListener(UndoListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->listeners.erase(
        std::remove(pImpl_->listeners.begin(), pImpl_->listeners.end(), listener),
        pImpl_->listeners.end());
}

void UndoService::setStateHashProvider(std::function<std::uint64_t()> provider) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->stateHashProvider = std::move(provider);
}

} // namespace cppmusic::services::undo

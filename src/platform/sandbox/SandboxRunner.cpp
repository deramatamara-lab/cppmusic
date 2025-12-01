/**
 * @file SandboxRunner.cpp
 * @brief Implementation of the sandbox runner (stub).
 */

#include "SandboxRunner.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <random>

namespace cppmusic::platform::sandbox {

const char* toString(SandboxStatus status) {
    switch (status) {
        case SandboxStatus::Starting: return "Starting";
        case SandboxStatus::Running: return "Running";
        case SandboxStatus::Unresponsive: return "Unresponsive";
        case SandboxStatus::Crashed: return "Crashed";
        case SandboxStatus::Terminated: return "Terminated";
    }
    return "Unknown";
}

struct SandboxEntry {
    SandboxId id = InvalidSandboxId;
    PluginInfo plugin;
    SandboxConfig config;
    SandboxStatus status = SandboxStatus::Starting;
    std::chrono::system_clock::time_point startTime;
};

struct SandboxRunner::Impl {
    std::map<SandboxId, SandboxEntry> sandboxes;
    std::vector<SandboxListener*> listeners;
    std::mutex mutex;
    std::chrono::milliseconds watchdogTimeout{5000};
    
    std::random_device rd;
    std::mt19937_64 rng{rd()};
    
    SandboxId generateId() {
        std::uniform_int_distribution<SandboxId> dist(1, UINT64_MAX);
        return dist(rng);
    }
};

SandboxRunner::SandboxRunner()
    : pImpl_(std::make_unique<Impl>()) {
}

SandboxRunner::~SandboxRunner() {
    terminateAll();
}

SandboxId SandboxRunner::spawn(const PluginInfo& plugin, const SandboxConfig& config) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    SandboxId id = pImpl_->generateId();
    
    SandboxEntry entry;
    entry.id = id;
    entry.plugin = plugin;
    entry.config = config;
    entry.status = SandboxStatus::Running;  // Stub: immediately running
    entry.startTime = std::chrono::system_clock::now();
    
    pImpl_->sandboxes[id] = entry;
    
    // Notify listeners
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onSandboxStarted(id, plugin);
        }
    }
    
    // TODO: Actually spawn a child process
    // - Fork/CreateProcess
    // - Set up IPC channels (shared memory, pipes)
    // - Apply security restrictions
    // - Start watchdog monitoring
    
    return id;
}

void SandboxRunner::terminate(SandboxId id) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    auto it = pImpl_->sandboxes.find(id);
    if (it == pImpl_->sandboxes.end()) {
        return;
    }
    
    it->second.status = SandboxStatus::Terminated;
    
    // Notify listeners
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onSandboxTerminated(id);
        }
    }
    
    pImpl_->sandboxes.erase(it);
    
    // TODO: Actually terminate the child process
    // - Send termination signal
    // - Wait for graceful shutdown
    // - Force kill if timeout
    // - Clean up IPC resources
}

void SandboxRunner::terminateAll() {
    std::vector<SandboxId> ids;
    
    {
        std::lock_guard<std::mutex> lock(pImpl_->mutex);
        for (const auto& [id, entry] : pImpl_->sandboxes) {
            ids.push_back(id);
        }
    }
    
    for (SandboxId id : ids) {
        terminate(id);
    }
}

SandboxStatus SandboxRunner::getStatus(SandboxId id) const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    auto it = pImpl_->sandboxes.find(id);
    if (it != pImpl_->sandboxes.end()) {
        return it->second.status;
    }
    
    return SandboxStatus::Terminated;
}

std::vector<SandboxId> SandboxRunner::getActiveSandboxes() const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    std::vector<SandboxId> ids;
    for (const auto& [id, entry] : pImpl_->sandboxes) {
        if (entry.status == SandboxStatus::Running ||
            entry.status == SandboxStatus::Starting) {
            ids.push_back(id);
        }
    }
    
    return ids;
}

const PluginInfo* SandboxRunner::getPluginInfo(SandboxId id) const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    auto it = pImpl_->sandboxes.find(id);
    if (it != pImpl_->sandboxes.end()) {
        return &it->second.plugin;
    }
    
    return nullptr;
}

void SandboxRunner::setWatchdogTimeout(std::chrono::milliseconds timeout) {
    pImpl_->watchdogTimeout = timeout;
}

std::chrono::milliseconds SandboxRunner::getWatchdogTimeout() const noexcept {
    return pImpl_->watchdogTimeout;
}

void SandboxRunner::addListener(SandboxListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    auto it = std::find(pImpl_->listeners.begin(), pImpl_->listeners.end(), listener);
    if (it == pImpl_->listeners.end()) {
        pImpl_->listeners.push_back(listener);
    }
}

void SandboxRunner::removeListener(SandboxListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->listeners.erase(
        std::remove(pImpl_->listeners.begin(), pImpl_->listeners.end(), listener),
        pImpl_->listeners.end());
}

} // namespace cppmusic::platform::sandbox

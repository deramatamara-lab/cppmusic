/**
 * @file SessionServer.cpp
 * @brief Implementation of the collaborative session server (placeholder).
 */

#include "SessionServer.hpp"
#include <algorithm>
#include <mutex>
#include <random>

namespace cppmusic::services::collab {

struct SessionServer::Impl {
    SessionId currentSession = InvalidSessionId;
    PeerId localPeerId = InvalidPeerId;
    ConnectionState connectionState = ConnectionState::Disconnected;
    
    std::vector<SessionListener*> listeners;
    std::vector<PeerPresence> connectedPeers;
    std::vector<Operation> pendingOperations;
    
    std::mutex mutex;
    
    // Generate random session/peer IDs
    std::random_device rd;
    std::mt19937_64 rng{rd()};
    
    SessionId generateSessionId() {
        std::uniform_int_distribution<SessionId> dist(1, UINT64_MAX);
        return dist(rng);
    }
    
    PeerId generatePeerId() {
        std::uniform_int_distribution<PeerId> dist(1, UINT32_MAX);
        return dist(rng);
    }
};

SessionServer::SessionServer()
    : pImpl_(std::make_unique<Impl>()) {
}

SessionServer::~SessionServer() {
    leaveSession();
}

SessionId SessionServer::createSession(const SessionConfig& config) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    // Generate IDs
    pImpl_->currentSession = pImpl_->generateSessionId();
    pImpl_->localPeerId = pImpl_->generatePeerId();
    pImpl_->connectionState = ConnectionState::Connected;
    
    // Add self to connected peers
    PeerPresence self;
    self.peerId = pImpl_->localPeerId;
    self.displayName = "Host";
    pImpl_->connectedPeers.push_back(self);
    
    // Notify listeners
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onConnectionStateChanged(ConnectionState::Connected);
        }
    }
    
    // Suppress unused parameter warning
    (void)config;
    
    return pImpl_->currentSession;
}

bool SessionServer::joinSession(SessionId sessionId, const std::string& address,
                                 const std::string& password) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    // Placeholder: In real implementation, would initiate network connection
    // For now, just simulate a successful local join
    
    pImpl_->currentSession = sessionId;
    pImpl_->localPeerId = pImpl_->generatePeerId();
    pImpl_->connectionState = ConnectionState::Connecting;
    
    // Notify connecting
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onConnectionStateChanged(ConnectionState::Connecting);
        }
    }
    
    // Simulate async connection success
    pImpl_->connectionState = ConnectionState::Connected;
    
    PeerPresence self;
    self.peerId = pImpl_->localPeerId;
    self.displayName = "Guest";
    pImpl_->connectedPeers.push_back(self);
    
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onConnectionStateChanged(ConnectionState::Connected);
        }
    }
    
    // Suppress unused parameter warnings
    (void)address;
    (void)password;
    
    return true;
}

void SessionServer::leaveSession() {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    if (pImpl_->currentSession == InvalidSessionId) {
        return;
    }
    
    pImpl_->currentSession = InvalidSessionId;
    pImpl_->localPeerId = InvalidPeerId;
    pImpl_->connectionState = ConnectionState::Disconnected;
    pImpl_->connectedPeers.clear();
    pImpl_->pendingOperations.clear();
    
    for (auto* listener : pImpl_->listeners) {
        if (listener) {
            listener->onConnectionStateChanged(ConnectionState::Disconnected);
        }
    }
}

SessionId SessionServer::getCurrentSession() const noexcept {
    return pImpl_->currentSession;
}

PeerId SessionServer::getLocalPeerId() const noexcept {
    return pImpl_->localPeerId;
}

ConnectionState SessionServer::getConnectionState() const noexcept {
    return pImpl_->connectionState;
}

void SessionServer::sendOperation(const Operation& op) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    if (pImpl_->connectionState != ConnectionState::Connected) {
        return;
    }
    
    // Placeholder: Queue operation for sending
    // In real implementation, would serialize and send over network
    pImpl_->pendingOperations.push_back(op);
}

void SessionServer::updatePresence(const PeerPresence& presence) {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    
    // Update local peer's presence
    for (auto& peer : pImpl_->connectedPeers) {
        if (peer.peerId == pImpl_->localPeerId) {
            peer = presence;
            peer.peerId = pImpl_->localPeerId;  // Preserve local ID
            break;
        }
    }
    
    // Placeholder: Would send presence update over network
}

std::vector<PeerPresence> SessionServer::getConnectedPeers() const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    return pImpl_->connectedPeers;
}

void SessionServer::addListener(SessionListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    auto it = std::find(pImpl_->listeners.begin(), pImpl_->listeners.end(), listener);
    if (it == pImpl_->listeners.end()) {
        pImpl_->listeners.push_back(listener);
    }
}

void SessionServer::removeListener(SessionListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(pImpl_->mutex);
    pImpl_->listeners.erase(
        std::remove(pImpl_->listeners.begin(), pImpl_->listeners.end(), listener),
        pImpl_->listeners.end());
}

} // namespace cppmusic::services::collab

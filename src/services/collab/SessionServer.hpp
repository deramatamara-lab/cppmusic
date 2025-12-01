#pragma once
/**
 * @file SessionServer.hpp
 * @brief Collaborative session server abstraction (placeholder).
 */

#include "PatternCRDT.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cppmusic::services::collab {

/**
 * @brief Unique identifier for a session.
 */
using SessionId = std::uint64_t;

/**
 * @brief Invalid session ID sentinel value.
 */
constexpr SessionId InvalidSessionId = 0;

/**
 * @brief Connection state for a session.
 */
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Error
};

/**
 * @brief Configuration for creating a session.
 */
struct SessionConfig {
    std::string sessionName;
    bool requirePassword = false;
    std::string password;  // Plaintext (will be hashed)
    std::uint32_t maxPeers = 8;
};

/**
 * @brief Presence information for a peer.
 */
struct PeerPresence {
    PeerId peerId = InvalidPeerId;
    std::string displayName;
    double cursorBeat = 0.0;
    bool isSelecting = false;
    double selectionStart = 0.0;
    double selectionEnd = 0.0;
};

/**
 * @brief Operation sent/received over the network.
 */
struct Operation {
    enum class Type {
        NoteInsert,
        NoteDelete,
        NoteUpdate,
        StateFull,
        StateDelta
    };
    
    Type type = Type::NoteInsert;
    std::vector<std::uint8_t> payload;
    PeerId sourcePeer = InvalidPeerId;
    std::uint64_t timestamp = 0;
};

/**
 * @brief Listener for session events.
 */
class SessionListener {
public:
    virtual ~SessionListener() = default;
    
    virtual void onConnectionStateChanged(ConnectionState state) = 0;
    virtual void onPeerJoined(const PeerPresence& peer) = 0;
    virtual void onPeerLeft(PeerId peerId) = 0;
    virtual void onRemoteOperation(const Operation& op) = 0;
    virtual void onPresenceUpdate(const PeerPresence& peer) = 0;
};

/**
 * @brief Collaborative session server (placeholder implementation).
 * 
 * This is a stub implementation that provides the interface for
 * future websocket/QUIC-based real-time collaboration.
 * 
 * Current implementation:
 * - Local-only operation (no network)
 * - Operations are queued but not actually sent
 * - Useful for interface validation and local testing
 */
class SessionServer {
public:
    SessionServer();
    ~SessionServer();
    
    // Non-copyable, non-movable
    SessionServer(const SessionServer&) = delete;
    SessionServer& operator=(const SessionServer&) = delete;
    SessionServer(SessionServer&&) = delete;
    SessionServer& operator=(SessionServer&&) = delete;
    
    // =========================================================================
    // Session Management
    // =========================================================================
    
    /**
     * @brief Create and host a new session.
     * @param config Session configuration.
     * @return The session ID, or InvalidSessionId on failure.
     */
    SessionId createSession(const SessionConfig& config);
    
    /**
     * @brief Join an existing session.
     * @param sessionId The session to join.
     * @param address Server address (placeholder).
     * @param password Session password (if required).
     * @return true if join initiated (async result via listener).
     */
    bool joinSession(SessionId sessionId, const std::string& address,
                     const std::string& password = "");
    
    /**
     * @brief Leave the current session.
     */
    void leaveSession();
    
    /**
     * @brief Get the current session ID.
     */
    [[nodiscard]] SessionId getCurrentSession() const noexcept;
    
    /**
     * @brief Get the local peer ID in the current session.
     */
    [[nodiscard]] PeerId getLocalPeerId() const noexcept;
    
    /**
     * @brief Get the current connection state.
     */
    [[nodiscard]] ConnectionState getConnectionState() const noexcept;
    
    // =========================================================================
    // Operations
    // =========================================================================
    
    /**
     * @brief Send an operation to all peers.
     */
    void sendOperation(const Operation& op);
    
    /**
     * @brief Update local presence information.
     */
    void updatePresence(const PeerPresence& presence);
    
    /**
     * @brief Get all connected peers.
     */
    [[nodiscard]] std::vector<PeerPresence> getConnectedPeers() const;
    
    // =========================================================================
    // Event Listeners
    // =========================================================================
    
    /**
     * @brief Add a session event listener.
     */
    void addListener(SessionListener* listener);
    
    /**
     * @brief Remove a session event listener.
     */
    void removeListener(SessionListener* listener);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace cppmusic::services::collab

#pragma once
/**
 * @file UndoService.hpp
 * @brief Command-based undo/redo service with delta compression.
 */

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::services::undo {

/**
 * @brief Represents the change from one state to another.
 */
struct StateDelta {
    enum class Type {
        PropertyChange,   ///< Single value change
        CollectionInsert, ///< Item added to collection
        CollectionRemove, ///< Item removed from collection
        CollectionMove,   ///< Item position changed
        Compound          ///< Multiple deltas combined
    };
    
    Type type = Type::PropertyChange;
    std::string path;  ///< JSON-like path to changed element
    std::vector<std::uint8_t> oldValue;
    std::vector<std::uint8_t> newValue;
    std::vector<StateDelta> children;  ///< For compound deltas
    
    /**
     * @brief Get the compressed size of this delta.
     */
    [[nodiscard]] std::size_t compressedSize() const noexcept;
    
    /**
     * @brief Serialize the delta to binary format.
     */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize from binary format.
     */
    static StateDelta deserialize(const std::vector<std::uint8_t>& data);
};

/**
 * @brief Abstract base class for undoable commands.
 */
class Command {
public:
    virtual ~Command() = default;
    
    /**
     * @brief Execute the command (apply change).
     */
    virtual void execute() = 0;
    
    /**
     * @brief Undo the command (revert change).
     */
    virtual void undo() = 0;
    
    /**
     * @brief Get a human-readable description.
     */
    [[nodiscard]] virtual std::string getDescription() const = 0;
    
    /**
     * @brief Serialize the command for persistence.
     */
    [[nodiscard]] virtual std::vector<std::uint8_t> serialize() const = 0;
    
    /**
     * @brief Get the state delta for this command.
     */
    [[nodiscard]] virtual StateDelta getDelta() const = 0;
    
    /**
     * @brief Check if this command can be merged with another.
     */
    [[nodiscard]] virtual bool canMergeWith(const Command& other) const;
    
    /**
     * @brief Merge another command into this one.
     * @return true if merged successfully.
     */
    virtual bool mergeWith(Command& other);
};

/**
 * @brief Entry in the undo history.
 */
struct UndoEntry {
    std::unique_ptr<Command> command;
    std::chrono::system_clock::time_point timestamp;
    std::uint64_t stateHash = 0;  ///< Hash after this command
    std::size_t batchId = 0;       ///< For grouping related commands
};

/**
 * @brief Listener for undo/redo events.
 */
class UndoListener {
public:
    virtual ~UndoListener() = default;
    virtual void onCommandExecuted(const Command& cmd) = 0;
    virtual void onUndo(const Command& cmd) = 0;
    virtual void onRedo(const Command& cmd) = 0;
    virtual void onHistoryCleared() = 0;
};

/**
 * @brief Central undo/redo service.
 * 
 * Provides:
 * - Command-based undo/redo
 * - Command batching for grouping related operations
 * - Delta compression for efficient storage
 * - State hash tracking for integrity verification
 */
class UndoService {
public:
    UndoService();
    ~UndoService();
    
    // Non-copyable, non-movable
    UndoService(const UndoService&) = delete;
    UndoService& operator=(const UndoService&) = delete;
    UndoService(UndoService&&) = delete;
    UndoService& operator=(UndoService&&) = delete;
    
    // =========================================================================
    // Command Execution
    // =========================================================================
    
    /**
     * @brief Execute a command and record for undo.
     */
    void execute(std::unique_ptr<Command> command);
    
    /**
     * @brief Begin a batch of commands (grouped as single undo).
     */
    void beginBatch(const std::string& description);
    
    /**
     * @brief End the current batch.
     */
    void endBatch();
    
    /**
     * @brief Check if currently in a batch.
     */
    [[nodiscard]] bool isInBatch() const noexcept;
    
    // =========================================================================
    // Undo/Redo Operations
    // =========================================================================
    
    /**
     * @brief Check if undo is available.
     */
    [[nodiscard]] bool canUndo() const noexcept;
    
    /**
     * @brief Check if redo is available.
     */
    [[nodiscard]] bool canRedo() const noexcept;
    
    /**
     * @brief Undo the last command (or batch).
     */
    void undo();
    
    /**
     * @brief Redo the last undone command (or batch).
     */
    void redo();
    
    /**
     * @brief Get the description of the command that would be undone.
     */
    [[nodiscard]] std::optional<std::string> getUndoDescription() const;
    
    /**
     * @brief Get the description of the command that would be redone.
     */
    [[nodiscard]] std::optional<std::string> getRedoDescription() const;
    
    // =========================================================================
    // History Navigation
    // =========================================================================
    
    /**
     * @brief Get the total history size.
     */
    [[nodiscard]] std::size_t getHistorySize() const noexcept;
    
    /**
     * @brief Get the current position in history.
     */
    [[nodiscard]] std::size_t getCurrentPosition() const noexcept;
    
    /**
     * @brief Jump to a specific position in history.
     */
    void jumpToPosition(std::size_t position);
    
    /**
     * @brief Get descriptions of all history entries.
     */
    [[nodiscard]] std::vector<std::string> getHistoryDescriptions() const;
    
    // =========================================================================
    // State Management
    // =========================================================================
    
    /**
     * @brief Capture a full state snapshot.
     */
    [[nodiscard]] std::vector<std::uint8_t> captureSnapshot() const;
    
    /**
     * @brief Restore from a snapshot.
     */
    void restoreSnapshot(const std::vector<std::uint8_t>& snapshot);
    
    /**
     * @brief Verify the integrity of the undo history.
     * @return true if all state hashes are valid.
     */
    [[nodiscard]] bool verifyIntegrity() const;
    
    /**
     * @brief Clear all history.
     */
    void clear();
    
    /**
     * @brief Set maximum history size.
     */
    void setMaxHistorySize(std::size_t size);
    
    /**
     * @brief Get maximum history size.
     */
    [[nodiscard]] std::size_t getMaxHistorySize() const noexcept;
    
    // =========================================================================
    // Event Listeners
    // =========================================================================
    
    /**
     * @brief Add an undo event listener.
     */
    void addListener(UndoListener* listener);
    
    /**
     * @brief Remove an undo event listener.
     */
    void removeListener(UndoListener* listener);
    
    // =========================================================================
    // State Hash Integration
    // =========================================================================
    
    /**
     * @brief Set the state hash provider function.
     */
    void setStateHashProvider(std::function<std::uint64_t()> provider);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace cppmusic::services::undo

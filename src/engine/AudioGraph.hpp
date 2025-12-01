#pragma once
/**
 * @file AudioGraph.hpp
 * @brief Audio processing graph with node registration, edge connections, 
 *        topology rebuild (topological sort), and block processing placeholder.
 *
 * This is the foundational engine skeleton for the cppmusic DAW.
 * No actual DSP implementations yet - this is a placeholder architecture.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cppmusic::engine {

/**
 * @brief Unique identifier for audio nodes.
 */
using NodeId = std::uint32_t;

/**
 * @brief Invalid node ID sentinel value.
 */
constexpr NodeId InvalidNodeId = 0;

/**
 * @brief Abstract base class for audio processing nodes.
 *
 * Each node can have multiple input and output ports.
 * Nodes are processed in topological order by the AudioGraph.
 */
class AudioNode {
public:
    virtual ~AudioNode() = default;

    /**
     * @brief Get the unique identifier for this node.
     */
    [[nodiscard]] virtual NodeId getId() const noexcept = 0;

    /**
     * @brief Get the human-readable name of this node.
     */
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;

    /**
     * @brief Get the number of input ports.
     */
    [[nodiscard]] virtual std::size_t getNumInputs() const noexcept = 0;

    /**
     * @brief Get the number of output ports.
     */
    [[nodiscard]] virtual std::size_t getNumOutputs() const noexcept = 0;

    /**
     * @brief Prepare the node for processing.
     * @param sampleRate The audio sample rate in Hz.
     * @param blockSize The maximum number of samples per block.
     */
    virtual void prepare(double sampleRate, std::size_t blockSize) = 0;

    /**
     * @brief Process a block of audio samples (placeholder - no DSP yet).
     * @param inputs Vector of input buffers (one per input port).
     * @param outputs Vector of output buffers (one per output port).
     * @param numSamples Number of samples to process.
     *
     * Real-time safe: must be noexcept, allocation-free, lock-free.
     */
    virtual void processBlock(
        const std::vector<const float*>& inputs,
        const std::vector<float*>& outputs,
        std::size_t numSamples) noexcept = 0;

    /**
     * @brief Release resources when processing stops.
     */
    virtual void release() = 0;
};

/**
 * @brief Represents a connection (edge) between two nodes in the audio graph.
 */
struct AudioEdge {
    NodeId sourceNode;
    std::size_t sourcePort;
    NodeId destNode;
    std::size_t destPort;

    bool operator==(const AudioEdge& other) const noexcept {
        return sourceNode == other.sourceNode &&
               sourcePort == other.sourcePort &&
               destNode == other.destNode &&
               destPort == other.destPort;
    }
};

/**
 * @brief Audio processing graph managing nodes, connections, and processing order.
 *
 * The graph maintains:
 * - A registry of audio nodes
 * - Edge connections between node ports
 * - A topologically sorted processing order
 *
 * Thread safety:
 * - Graph modification (add/remove nodes/edges) must be done from the non-audio thread.
 * - processBlock() is called from the audio thread and is real-time safe.
 */
class AudioGraph {
public:
    AudioGraph();
    ~AudioGraph();

    // Non-copyable, non-movable (owns unique resources)
    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;
    AudioGraph(AudioGraph&&) = delete;
    AudioGraph& operator=(AudioGraph&&) = delete;

    // =========================================================================
    // Node Management (call from non-audio thread)
    // =========================================================================

    /**
     * @brief Register a node in the graph.
     * @param node Unique pointer to the audio node.
     * @return The node ID of the registered node, or InvalidNodeId on failure.
     */
    NodeId registerNode(std::unique_ptr<AudioNode> node);

    /**
     * @brief Unregister and remove a node from the graph.
     * @param nodeId The ID of the node to remove.
     * @return true if the node was removed, false if not found.
     */
    bool unregisterNode(NodeId nodeId);

    /**
     * @brief Get a pointer to a registered node.
     * @param nodeId The ID of the node.
     * @return Pointer to the node, or nullptr if not found.
     */
    [[nodiscard]] AudioNode* getNode(NodeId nodeId) const;

    /**
     * @brief Get the number of registered nodes.
     */
    [[nodiscard]] std::size_t getNodeCount() const noexcept;

    // =========================================================================
    // Edge Management (call from non-audio thread)
    // =========================================================================

    /**
     * @brief Connect two nodes via their ports.
     * @param sourceNode The source node ID.
     * @param sourcePort The output port index on the source node.
     * @param destNode The destination node ID.
     * @param destPort The input port index on the destination node.
     * @return true if the connection was made, false on failure.
     */
    bool connect(NodeId sourceNode, std::size_t sourcePort,
                 NodeId destNode, std::size_t destPort);

    /**
     * @brief Disconnect two nodes.
     * @param sourceNode The source node ID.
     * @param sourcePort The output port index on the source node.
     * @param destNode The destination node ID.
     * @param destPort The input port index on the destination node.
     * @return true if the connection was removed, false if not found.
     */
    bool disconnect(NodeId sourceNode, std::size_t sourcePort,
                    NodeId destNode, std::size_t destPort);

    /**
     * @brief Get all edges in the graph.
     */
    [[nodiscard]] const std::vector<AudioEdge>& getEdges() const noexcept;

    // =========================================================================
    // Topology Management
    // =========================================================================

    /**
     * @brief Rebuild the processing order using topological sort.
     *
     * Must be called after any graph modifications before processing.
     * @return true if the topology was rebuilt successfully (no cycles),
     *         false if a cycle was detected.
     */
    bool rebuildTopology();

    /**
     * @brief Check if the graph has a valid processing topology.
     */
    [[nodiscard]] bool hasValidTopology() const noexcept;

    /**
     * @brief Get the processing order (topologically sorted node IDs).
     */
    [[nodiscard]] const std::vector<NodeId>& getProcessingOrder() const noexcept;

    // =========================================================================
    // Audio Processing
    // =========================================================================

    /**
     * @brief Prepare all nodes for processing.
     * @param sampleRate The audio sample rate in Hz.
     * @param blockSize The maximum number of samples per block.
     */
    void prepare(double sampleRate, std::size_t blockSize);

    /**
     * @brief Process a block of audio through the graph.
     * @param numSamples Number of samples to process.
     *
     * Real-time safe: noexcept, allocation-free, lock-free.
     * Processes nodes in topological order.
     */
    void processBlock(std::size_t numSamples) noexcept;

    /**
     * @brief Release resources for all nodes.
     */
    void release();

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace cppmusic::engine

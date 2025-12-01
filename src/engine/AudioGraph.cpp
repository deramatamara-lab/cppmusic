/**
 * @file AudioGraph.cpp
 * @brief Implementation of the audio processing graph.
 */

#include "AudioGraph.hpp"
#include <algorithm>
#include <queue>
#include <unordered_set>

namespace cppmusic::engine {

/**
 * @brief Private implementation (PIMPL) for AudioGraph.
 */
struct AudioGraph::Impl {
    /// Registered nodes indexed by their ID.
    std::unordered_map<NodeId, std::unique_ptr<AudioNode>> nodes;

    /// All edges (connections) in the graph.
    std::vector<AudioEdge> edges;

    /// Topologically sorted processing order.
    std::vector<NodeId> processingOrder;

    /// Whether the current topology is valid.
    bool topologyValid = false;

    /// Sample rate for audio processing.
    double sampleRate = 44100.0;

    /// Block size for audio processing.
    std::size_t blockSize = 512;

    /// Next available node ID.
    NodeId nextNodeId = 1;

    /**
     * @brief Perform topological sort using Kahn's algorithm.
     * @return true if successful (no cycles), false if a cycle is detected.
     */
    bool topologicalSort() {
        processingOrder.clear();

        if (nodes.empty()) {
            topologyValid = true;
            return true;
        }

        // Build adjacency list and in-degree map
        std::unordered_map<NodeId, std::vector<NodeId>> adjacency;
        std::unordered_map<NodeId, std::size_t> inDegree;

        // Initialize in-degree for all nodes
        for (const auto& [nodeId, node] : nodes) {
            inDegree[nodeId] = 0;
            adjacency[nodeId] = {};
        }

        // Build adjacency list from edges
        for (const auto& edge : edges) {
            adjacency[edge.sourceNode].push_back(edge.destNode);
            ++inDegree[edge.destNode];
        }

        // Queue of nodes with no incoming edges
        std::queue<NodeId> zeroInDegree;
        for (const auto& [nodeId, degree] : inDegree) {
            if (degree == 0) {
                zeroInDegree.push(nodeId);
            }
        }

        // Process nodes in topological order
        while (!zeroInDegree.empty()) {
            NodeId current = zeroInDegree.front();
            zeroInDegree.pop();
            processingOrder.push_back(current);

            for (NodeId neighbor : adjacency[current]) {
                --inDegree[neighbor];
                if (inDegree[neighbor] == 0) {
                    zeroInDegree.push(neighbor);
                }
            }
        }

        // If we didn't process all nodes, there's a cycle
        if (processingOrder.size() != nodes.size()) {
            processingOrder.clear();
            topologyValid = false;
            return false;
        }

        topologyValid = true;
        return true;
    }
};

AudioGraph::AudioGraph()
    : pImpl(std::make_unique<Impl>()) {
}

AudioGraph::~AudioGraph() = default;

// =============================================================================
// Node Management
// =============================================================================

NodeId AudioGraph::registerNode(std::unique_ptr<AudioNode> node) {
    if (!node) {
        return InvalidNodeId;
    }

    NodeId nodeId = node->getId();
    if (nodeId == InvalidNodeId) {
        // Assign a new ID if the node doesn't have one
        nodeId = pImpl->nextNodeId++;
    }

    // Check for duplicate ID
    if (pImpl->nodes.find(nodeId) != pImpl->nodes.end()) {
        return InvalidNodeId;
    }

    pImpl->nodes[nodeId] = std::move(node);
    pImpl->topologyValid = false;
    return nodeId;
}

bool AudioGraph::unregisterNode(NodeId nodeId) {
    auto it = pImpl->nodes.find(nodeId);
    if (it == pImpl->nodes.end()) {
        return false;
    }

    // Remove all edges connected to this node
    pImpl->edges.erase(
        std::remove_if(pImpl->edges.begin(), pImpl->edges.end(),
            [nodeId](const AudioEdge& edge) {
                return edge.sourceNode == nodeId || edge.destNode == nodeId;
            }),
        pImpl->edges.end());

    pImpl->nodes.erase(it);
    pImpl->topologyValid = false;
    return true;
}

AudioNode* AudioGraph::getNode(NodeId nodeId) const {
    auto it = pImpl->nodes.find(nodeId);
    if (it != pImpl->nodes.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::size_t AudioGraph::getNodeCount() const noexcept {
    return pImpl->nodes.size();
}

// =============================================================================
// Edge Management
// =============================================================================

bool AudioGraph::connect(NodeId sourceNode, std::size_t sourcePort,
                          NodeId destNode, std::size_t destPort) {
    // Validate nodes exist
    auto srcIt = pImpl->nodes.find(sourceNode);
    auto dstIt = pImpl->nodes.find(destNode);
    if (srcIt == pImpl->nodes.end() || dstIt == pImpl->nodes.end()) {
        return false;
    }

    // Validate port indices
    if (sourcePort >= srcIt->second->getNumOutputs() ||
        destPort >= dstIt->second->getNumInputs()) {
        return false;
    }

    // Check for duplicate edge
    AudioEdge newEdge{sourceNode, sourcePort, destNode, destPort};
    for (const auto& edge : pImpl->edges) {
        if (edge == newEdge) {
            return false; // Already connected
        }
    }

    pImpl->edges.push_back(newEdge);
    pImpl->topologyValid = false;
    return true;
}

bool AudioGraph::disconnect(NodeId sourceNode, std::size_t sourcePort,
                             NodeId destNode, std::size_t destPort) {
    AudioEdge targetEdge{sourceNode, sourcePort, destNode, destPort};
    
    auto it = std::find(pImpl->edges.begin(), pImpl->edges.end(), targetEdge);
    if (it != pImpl->edges.end()) {
        pImpl->edges.erase(it);
        pImpl->topologyValid = false;
        return true;
    }
    return false;
}

const std::vector<AudioEdge>& AudioGraph::getEdges() const noexcept {
    return pImpl->edges;
}

// =============================================================================
// Topology Management
// =============================================================================

bool AudioGraph::rebuildTopology() {
    return pImpl->topologicalSort();
}

bool AudioGraph::hasValidTopology() const noexcept {
    return pImpl->topologyValid;
}

const std::vector<NodeId>& AudioGraph::getProcessingOrder() const noexcept {
    return pImpl->processingOrder;
}

// =============================================================================
// Audio Processing
// =============================================================================

void AudioGraph::prepare(double sampleRate, std::size_t blockSize) {
    pImpl->sampleRate = sampleRate;
    pImpl->blockSize = blockSize;

    for (auto& [nodeId, node] : pImpl->nodes) {
        node->prepare(sampleRate, blockSize);
    }
}

void AudioGraph::processBlock(std::size_t numSamples) noexcept {
    if (!pImpl->topologyValid) {
        return;
    }

    // Placeholder: Process nodes in topological order.
    // Real implementation would route buffers between nodes according to edges.
    // For now, this is a skeleton that just calls each node's processBlock.
    for (NodeId nodeId : pImpl->processingOrder) {
        auto* node = getNode(nodeId);
        if (node) {
            // Placeholder: Empty input/output buffers for now
            std::vector<const float*> inputs;
            std::vector<float*> outputs;
            node->processBlock(inputs, outputs, numSamples);
        }
    }
}

void AudioGraph::release() {
    for (auto& [nodeId, node] : pImpl->nodes) {
        node->release();
    }
}

} // namespace cppmusic::engine

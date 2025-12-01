/**
 * @file DiagnosticsOverlay.cpp
 * @brief Implementation of diagnostics and profiling overlay
 */

#include "DiagnosticsOverlay.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <thread>

namespace daw::ui::diagnostics
{

// ============================================================================
// ScopedTimer Implementation
// ============================================================================

ScopedTimer::ScopedTimer(const std::string& name, const std::string& category)
    : name_(name), category_(category)
    , startTime_(std::chrono::high_resolution_clock::now())
{
}

ScopedTimer::~ScopedTimer()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto startUs = std::chrono::duration_cast<std::chrono::microseconds>(
        startTime_.time_since_epoch()).count();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime_).count();
    
    TimingEvent event;
    event.name = name_;
    event.category = category_;
    event.startTimeUs = static_cast<uint64_t>(startUs);
    event.durationUs = static_cast<uint64_t>(durationUs);
    event.threadId = static_cast<int>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFF);
    
    getGlobalDiagnostics().recordEvent(event);
}

// ============================================================================
// DiagnosticsManager Implementation
// ============================================================================

DiagnosticsManager::DiagnosticsManager()
{
    frameTimeHistory_.resize(historySize_, 0.0f);
}

DiagnosticsManager& getGlobalDiagnostics()
{
    static DiagnosticsManager instance;
    return instance;
}

void DiagnosticsManager::beginFrame()
{
    frameStartTime_ = std::chrono::high_resolution_clock::now();
    currentStats_ = FrameStats{};
}

void DiagnosticsManager::endFrame()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(endTime - frameStartTime_);
    currentStats_.frameTimeMs = duration.count();
    currentStats_.cpuTimeMs = duration.count();  // GPU timing requires GL queries
    
    // Update history
    frameTimeHistory_.pop_front();
    frameTimeHistory_.push_back(currentStats_.frameTimeMs);
    
    updateMetrics();
}

void DiagnosticsManager::recordEvent(const TimingEvent& event)
{
    if (capturing_) {
        std::lock_guard<std::mutex> lock(traceMutex_);
        capturedEvents_.push_back(event);
    }
}

void DiagnosticsManager::recordAudioTiming(float occupancy)
{
    currentStats_.audioThreadOccupancy = std::clamp(occupancy, 0.0f, 1.0f);
}

void DiagnosticsManager::updateMetrics()
{
    // Calculate FPS
    if (currentStats_.frameTimeMs > 0.0f) {
        fps_ = 1000.0f / currentStats_.frameTimeMs;
    }
    
    // Calculate average
    float sum = std::accumulate(frameTimeHistory_.begin(), frameTimeHistory_.end(), 0.0f);
    avgFrameTimeMs_ = sum / static_cast<float>(frameTimeHistory_.size());
    
    // Calculate 99th percentile
    std::vector<float> sorted(frameTimeHistory_.begin(), frameTimeHistory_.end());
    std::sort(sorted.begin(), sorted.end());
    std::size_t p99Index = static_cast<std::size_t>(
        static_cast<double>(sorted.size()) * 0.99);
    p99Index = std::min(p99Index, sorted.size() - 1);
    p99FrameTimeMs_ = sorted[p99Index];
}

void DiagnosticsManager::startTraceCapture()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    capturedEvents_.clear();
    capturing_ = true;
}

void DiagnosticsManager::stopTraceCapture()
{
    capturing_ = false;
}

bool DiagnosticsManager::exportTrace(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << getTraceJSON();
    return true;
}

std::string DiagnosticsManager::getTraceJSON() const
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    
    std::ostringstream ss;
    ss << "{\n  \"traceEvents\": [\n";
    
    bool first = true;
    for (const auto& event : capturedEvents_) {
        if (!first) ss << ",\n";
        first = false;
        
        ss << "    {"
           << "\"name\": \"" << event.name << "\", "
           << "\"cat\": \"" << event.category << "\", "
           << "\"ph\": \"X\", "
           << "\"ts\": " << event.startTimeUs << ", "
           << "\"dur\": " << event.durationUs << ", "
           << "\"pid\": 1, "
           << "\"tid\": " << event.threadId;
        
        if (!event.args.empty()) {
            ss << ", \"args\": " << event.args;
        }
        
        ss << "}";
    }
    
    ss << "\n  ],\n"
       << "  \"displayTimeUnit\": \"ms\",\n"
       << "  \"systemTraceEvents\": \"SystemTraceData\",\n"
       << "  \"otherData\": {\n"
       << "    \"version\": \"cppmusic DAW Trace v1.0\"\n"
       << "  }\n"
       << "}\n";
    
    return ss.str();
}

void DiagnosticsManager::clearTrace()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    capturedEvents_.clear();
}

void DiagnosticsManager::pushUndoRecord(const UndoRecord& record)
{
    undoHistory_.push_back(record);
    if (undoHistory_.size() > 100) {
        undoHistory_.erase(undoHistory_.begin());
    }
}

// ============================================================================
// DiagnosticsOverlay Implementation
// ============================================================================

void DiagnosticsOverlay::draw(bool& visible, DiagnosticsManager& diagnostics)
{
    if (!visible) return;
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;
    
    // Position in top-right corner
    const float padding = 10.0f;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    ImVec2 windowPos(workPos.x + workSize.x - padding, workPos.y + padding);
    ImVec2 windowPosPivot(1.0f, 0.0f);
    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    ImGui::SetNextWindowBgAlpha(overlayAlpha);
    
    if (ImGui::Begin("##DiagnosticsOverlay", &visible, flags)) {
        const auto& stats = diagnostics.getCurrentStats();
        
        // FPS and frame time
        ImVec4 fpsColor = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
        if (stats.frameTimeMs > 16.67f) fpsColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
        if (stats.frameTimeMs > 33.33f) fpsColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        
        ImGui::TextColored(fpsColor, "%.1f FPS", diagnostics.getFPS());
        ImGui::SameLine();
        ImGui::Text("(%.2f ms)", stats.frameTimeMs);
        
        // Quick stats
        ImGui::Text("Avg: %.2f ms | P99: %.2f ms",
                   diagnostics.getAverageFrameTime(),
                   diagnostics.get99thPercentileFrameTime());
        
        // Draw calls and vertices
        ImGui::Text("Draw: %d | Verts: %d", stats.drawCalls, stats.vertexCount);
        
        // Audio thread
        ImVec4 audioColor = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
        if (stats.audioThreadOccupancy > 0.7f) audioColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
        if (stats.audioThreadOccupancy > 0.9f) audioColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(audioColor, "Audio: %.0f%%", stats.audioThreadOccupancy * 100.0f);
        
        // Virtualization stats
        if (stats.visibleNotes > 0 || stats.visibleClips > 0) {
            ImGui::Text("Visible: %d notes, %d clips", stats.visibleNotes, stats.visibleClips);
        }
        
        // Reactive signals
        if (stats.dirtySignals > 0) {
            ImGui::Text("Dirty signals: %d", stats.dirtySignals);
        }
        
        // Collapsible sections
        if (ImGui::CollapsingHeader("Graph", showGraph ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
            showGraph = true;
            drawFrameTimeGraph(diagnostics);
        } else {
            showGraph = false;
        }
        
        if (ImGui::CollapsingHeader("Details")) {
            showDetails = true;
            drawMetricsDetails(diagnostics);
        } else {
            showDetails = false;
        }
        
        if (ImGui::CollapsingHeader("Trace")) {
            showTrace = true;
            drawTraceControls(diagnostics);
        } else {
            showTrace = false;
        }
        
        if (ImGui::CollapsingHeader("Undo History")) {
            showUndoHistory = true;
            drawUndoIntrospection(diagnostics);
        } else {
            showUndoHistory = false;
        }
    }
    ImGui::End();
}

void DiagnosticsOverlay::drawFrameTimeGraph(DiagnosticsManager& diagnostics)
{
    const auto& history = diagnostics.getFrameTimeHistory();
    if (history.empty()) return;
    
    // Find max for scaling
    float maxTime = *std::max_element(history.begin(), history.end());
    maxTime = std::max(maxTime, 16.67f);  // At least 60fps scale
    
    // Convert to array for ImGui
    std::vector<float> values(history.begin(), history.end());
    
    char overlay[64];
    snprintf(overlay, sizeof(overlay), "Max: %.2f ms", maxTime);
    
    ImGui::PlotLines("##FrameTime", values.data(),
                     static_cast<int>(values.size()), 0,
                     overlay, 0.0f, maxTime * 1.2f,
                     ImVec2(200, graphHeight));
    
    // Target lines
    ImVec2 graphMin = ImGui::GetItemRectMin();
    ImVec2 graphMax = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // 16.67ms line (60fps)
    float y60 = graphMax.y - (graphMax.y - graphMin.y) * (16.67f / (maxTime * 1.2f));
    drawList->AddLine(ImVec2(graphMin.x, y60), ImVec2(graphMax.x, y60),
                     IM_COL32(100, 200, 100, 100), 1.0f);
    
    // 33.33ms line (30fps)
    if (maxTime > 20.0f) {
        float y30 = graphMax.y - (graphMax.y - graphMin.y) * (33.33f / (maxTime * 1.2f));
        drawList->AddLine(ImVec2(graphMin.x, y30), ImVec2(graphMax.x, y30),
                         IM_COL32(200, 200, 100, 100), 1.0f);
    }
}

void DiagnosticsOverlay::drawMetricsDetails(DiagnosticsManager& diagnostics)
{
    const auto& stats = diagnostics.getCurrentStats();
    
    ImGui::Text("CPU Time: %.3f ms", stats.cpuTimeMs);
    ImGui::Text("GPU Time: %.3f ms", stats.gpuTimeMs);
    ImGui::Text("Triangles: %d", stats.triangleCount);
    
    if (stats.allocatedBytes > 0) {
        float mb = static_cast<float>(stats.allocatedBytes) / (1024.0f * 1024.0f);
        ImGui::Text("Allocations: %.2f MB", mb);
    }
    
    ImGui::Separator();
    
    // Performance budget
    float budget = 4.0f;  // Target < 4ms mean
    float usedPct = (diagnostics.getAverageFrameTime() / budget) * 100.0f;
    
    ImVec4 budgetColor = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
    if (usedPct > 75.0f) budgetColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
    if (usedPct > 100.0f) budgetColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
    
    ImGui::TextColored(budgetColor, "Budget: %.0f%% of %.1fms target", usedPct, budget);
}

void DiagnosticsOverlay::drawTraceControls(DiagnosticsManager& diagnostics)
{
    if (diagnostics.isCapturing()) {
        if (ImGui::Button("Stop Capture")) {
            diagnostics.stopTraceCapture();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "RECORDING");
    } else {
        if (ImGui::Button("Start Capture")) {
            diagnostics.startTraceCapture();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Export JSON")) {
        diagnostics.exportTrace("/tmp/daw_trace.json");
        // In a real app, use a file dialog
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        diagnostics.clearTrace();
    }
    
    ImGui::TextDisabled("Export to Chrome trace format (chrome://tracing)");
}

void DiagnosticsOverlay::drawUndoIntrospection(DiagnosticsManager& diagnostics)
{
    const auto& history = diagnostics.getUndoHistory();
    
    if (history.empty()) {
        ImGui::TextDisabled("No undo history");
        return;
    }
    
    if (ImGui::BeginChild("UndoList", ImVec2(0, 100), true)) {
        for (auto it = history.rbegin(); it != history.rend(); ++it) {
            const auto& record = *it;
            
            ImVec4 color = record.canUndo ? ImVec4(0.8f, 0.8f, 0.8f, 1.0f) :
                                            ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::TextColored(color, "[%llu] %s",
                              static_cast<unsigned long long>(record.id),
                              record.description.c_str());
            if (!record.timestamp.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", record.timestamp.c_str());
            }
        }
    }
    ImGui::EndChild();
    
    if (ImGui::Button("Clear History")) {
        diagnostics.clearUndoHistory();
    }
}

} // namespace daw::ui::diagnostics

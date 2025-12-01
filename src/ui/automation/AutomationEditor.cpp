/**
 * @file AutomationEditor.cpp
 * @brief Automation lanes editor with layer support (stub)
 */

#include "AutomationEditor.h"

namespace daw::ui::automation {

class AutomationEditor::Impl {
public:
    struct AutomationPoint {
        double time{0.0};
        float value{0.5f};
        int curveType{0};  // 0=linear, 1=smooth, 2=step
    };
    
    struct AutomationLane {
        juce::String parameterId;
        juce::String parameterName;
        std::vector<AutomationPoint> points;
        juce::Colour color{0xff4080ff};
        bool visible{true};
        int layerType{0};  // 0=Base, 1=Override, 2=Macro
    };
    
    std::vector<AutomationLane> lanes;
    int selectedLane{-1};
    int selectedPoint{-1};
    
    double viewStartTime{0.0};
    double viewEndTime{16.0};
    float zoomLevel{1.0f};
    bool showGrid{true};
    bool snapToGrid{true};
    double gridSize{0.25};  // Quarter beat
};

AutomationEditor::AutomationEditor()
    : impl_(std::make_unique<Impl>()) {
}

AutomationEditor::~AutomationEditor() = default;

void AutomationEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Grid
    if (impl_->showGrid) {
        g.setColour(juce::Colour(0xff2a2a2a));
        double viewDuration = impl_->viewEndTime - impl_->viewStartTime;
        double gridStep = impl_->gridSize;
        
        for (double t = 0; t < viewDuration; t += gridStep) {
            float x = static_cast<float>(t / viewDuration * bounds.getWidth());
            g.drawVerticalLine(static_cast<int>(x), 0, bounds.getHeight());
        }
        
        // Value grid
        for (float v = 0.0f; v <= 1.0f; v += 0.1f) {
            float y = (1.0f - v) * bounds.getHeight();
            g.drawHorizontalLine(static_cast<int>(y), 0, bounds.getWidth());
        }
    }
    
    // Draw lanes
    float laneHeight = bounds.getHeight() / std::max(1, static_cast<int>(impl_->lanes.size()));
    int laneIndex = 0;
    
    for (const auto& lane : impl_->lanes) {
        if (!lane.visible) continue;
        
        auto laneRect = bounds.withTop(laneIndex * laneHeight).withHeight(laneHeight);
        
        // Lane background
        g.setColour(lane.color.withAlpha(0.1f));
        g.fillRect(laneRect);
        
        // Lane label
        g.setColour(lane.color);
        g.drawText(lane.parameterName, laneRect.removeFromTop(20), 
                   juce::Justification::left);
        
        // Draw automation curve
        if (lane.points.size() >= 2) {
            juce::Path path;
            bool first = true;
            
            double viewDuration = impl_->viewEndTime - impl_->viewStartTime;
            
            for (const auto& point : lane.points) {
                float x = static_cast<float>((point.time - impl_->viewStartTime) / 
                                              viewDuration * bounds.getWidth());
                float y = laneRect.getY() + (1.0f - point.value) * laneRect.getHeight();
                
                if (first) {
                    path.startNewSubPath(x, y);
                    first = false;
                } else {
                    path.lineTo(x, y);
                }
            }
            
            g.setColour(lane.color);
            g.strokePath(path, juce::PathStrokeType(2.0f));
            
            // Draw points
            for (const auto& point : lane.points) {
                float x = static_cast<float>((point.time - impl_->viewStartTime) / 
                                              viewDuration * bounds.getWidth());
                float y = laneRect.getY() + (1.0f - point.value) * laneRect.getHeight();
                
                g.fillEllipse(x - 4, y - 4, 8, 8);
            }
        }
        
        ++laneIndex;
    }
    
    // Empty state
    if (impl_->lanes.empty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("Automation Editor\nAdd parameters to automate",
                   bounds, juce::Justification::centred);
    }
}

void AutomationEditor::resized() {
    // Layout handled in paint
}

void AutomationEditor::setViewRange(double startTime, double endTime) {
    impl_->viewStartTime = startTime;
    impl_->viewEndTime = endTime;
    repaint();
}

void AutomationEditor::setZoomLevel(float zoom) {
    impl_->zoomLevel = juce::jlimit(0.1f, 10.0f, zoom);
    repaint();
}

float AutomationEditor::getZoomLevel() const {
    return impl_->zoomLevel;
}

void AutomationEditor::setShowGrid(bool show) {
    impl_->showGrid = show;
    repaint();
}

bool AutomationEditor::isGridVisible() const {
    return impl_->showGrid;
}

void AutomationEditor::setSnapToGrid(bool snap) {
    impl_->snapToGrid = snap;
}

bool AutomationEditor::isSnapToGridEnabled() const {
    return impl_->snapToGrid;
}

void AutomationEditor::setGridSize(double beats) {
    impl_->gridSize = beats;
    repaint();
}

double AutomationEditor::getGridSize() const {
    return impl_->gridSize;
}

}  // namespace daw::ui::automation

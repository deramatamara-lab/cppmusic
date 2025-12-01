/**
 * @file ParameterGraphWidget.cpp
 * @brief Parameter modulation graph visualization (stub)
 */

#include "ParameterGraphWidget.h"

namespace daw::ui::graphs {

class ParameterGraphWidget::Impl {
public:
    struct Node {
        juce::String id;
        juce::String name;
        float x{0.0f}, y{0.0f};
        float value{0.0f};
        juce::Colour color{0xff4080ff};
    };
    
    struct Connection {
        juce::String sourceId;
        juce::String targetId;
        float amount{1.0f};
    };
    
    std::vector<Node> nodes;
    std::vector<Connection> connections;
    
    float zoomLevel{1.0f};
    float panX{0.0f}, panY{0.0f};
    bool showValues{true};
};

ParameterGraphWidget::ParameterGraphWidget()
    : impl_(std::make_unique<Impl>()) {
}

ParameterGraphWidget::~ParameterGraphWidget() = default;

void ParameterGraphWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Grid
    g.setColour(juce::Colour(0xff2a2a2a));
    for (int x = 0; x < bounds.getWidth(); x += 50) {
        g.drawVerticalLine(x, 0, bounds.getHeight());
    }
    for (int y = 0; y < bounds.getHeight(); y += 50) {
        g.drawHorizontalLine(y, 0, bounds.getWidth());
    }
    
    // Draw connections
    g.setColour(juce::Colour(0xff606060));
    for (const auto& conn : impl_->connections) {
        // Find source and target nodes
        auto findNode = [this](const juce::String& id) -> const Impl::Node* {
            for (const auto& node : impl_->nodes) {
                if (node.id == id) return &node;
            }
            return nullptr;
        };
        
        auto* src = findNode(conn.sourceId);
        auto* dst = findNode(conn.targetId);
        
        if (src && dst) {
            juce::Path path;
            float x1 = src->x * impl_->zoomLevel + impl_->panX + bounds.getCentreX();
            float y1 = src->y * impl_->zoomLevel + impl_->panY + bounds.getCentreY();
            float x2 = dst->x * impl_->zoomLevel + impl_->panX + bounds.getCentreX();
            float y2 = dst->y * impl_->zoomLevel + impl_->panY + bounds.getCentreY();
            
            path.startNewSubPath(x1, y1);
            path.cubicTo(x1 + 50, y1, x2 - 50, y2, x2, y2);
            
            g.setColour(juce::Colour(0xff4080ff).withAlpha(conn.amount));
            g.strokePath(path, juce::PathStrokeType(2.0f));
        }
    }
    
    // Draw nodes
    for (const auto& node : impl_->nodes) {
        float x = node.x * impl_->zoomLevel + impl_->panX + bounds.getCentreX();
        float y = node.y * impl_->zoomLevel + impl_->panY + bounds.getCentreY();
        float size = 30.0f * impl_->zoomLevel;
        
        juce::Rectangle<float> nodeRect(x - size/2, y - size/2, size, size);
        
        g.setColour(node.color);
        g.fillRoundedRectangle(nodeRect, 5.0f);
        
        g.setColour(juce::Colours::white);
        g.drawText(node.name, nodeRect.translated(0, size + 5), 
                   juce::Justification::centredTop);
        
        if (impl_->showValues) {
            g.drawText(juce::String(node.value, 2), nodeRect,
                       juce::Justification::centred);
        }
    }
    
    // Help text for empty graph
    if (impl_->nodes.empty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("Parameter Graph (empty)\nAdd parameters and modulations to visualize",
                   bounds, juce::Justification::centred);
    }
}

void ParameterGraphWidget::resized() {
    // Auto-center
}

void ParameterGraphWidget::setZoomLevel(float zoom) {
    impl_->zoomLevel = juce::jlimit(0.1f, 5.0f, zoom);
    repaint();
}

float ParameterGraphWidget::getZoomLevel() const {
    return impl_->zoomLevel;
}

void ParameterGraphWidget::setPan(float x, float y) {
    impl_->panX = x;
    impl_->panY = y;
    repaint();
}

void ParameterGraphWidget::resetView() {
    impl_->zoomLevel = 1.0f;
    impl_->panX = 0.0f;
    impl_->panY = 0.0f;
    repaint();
}

void ParameterGraphWidget::setShowValues(bool show) {
    impl_->showValues = show;
    repaint();
}

bool ParameterGraphWidget::isShowingValues() const {
    return impl_->showValues;
}

}  // namespace daw::ui::graphs

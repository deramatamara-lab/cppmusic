/**
 * @file PluginHealthWidget.cpp
 * @brief Plugin health and sandbox status widget (stub)
 */

#include "PluginHealthWidget.h"

namespace daw::ui::plugins {

class PluginHealthWidget::Impl {
public:
    struct PluginInfo {
        juce::String id;
        juce::String name;
        juce::String vendor;
        juce::String format;  // VST3, AU, etc.
        
        // Health metrics
        enum class Status { Healthy, Warning, Error, Crashed, Sandboxed };
        Status status{Status::Healthy};
        
        float cpuUsage{0.0f};
        float avgLatencyMs{0.0f};
        float maxLatencyMs{0.0f};
        int crashCount{0};
        
        bool sandboxed{false};
        bool suspended{false};
    };
    
    std::vector<PluginInfo> plugins;
    int selectedIndex{-1};
    bool showSandboxedOnly{false};
};

PluginHealthWidget::PluginHealthWidget()
    : impl_(std::make_unique<Impl>()) {
}

PluginHealthWidget::~PluginHealthWidget() = default;

void PluginHealthWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Header
    auto header = bounds.removeFromTop(35);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(header);
    
    g.setColour(juce::Colours::white);
    g.drawText("Plugin Health Monitor", header.reduced(10, 0), 
               juce::Justification::centredLeft);
    
    bounds.removeFromTop(5);
    
    // Column headers
    auto colHeader = bounds.removeFromTop(25);
    g.setColour(juce::Colour(0xff606060));
    g.drawText("Plugin", colHeader.withWidth(200), juce::Justification::centredLeft);
    g.drawText("Status", colHeader.withX(200).withWidth(80), juce::Justification::centred);
    g.drawText("CPU", colHeader.withX(280).withWidth(60), juce::Justification::centred);
    g.drawText("Latency", colHeader.withX(340).withWidth(80), juce::Justification::centred);
    g.drawText("Sandbox", colHeader.withX(420).withWidth(70), juce::Justification::centred);
    
    bounds.removeFromTop(5);
    
    // Plugin list
    if (impl_->plugins.empty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("No plugins loaded\nAdd plugins to monitor their health",
                   bounds, juce::Justification::centred);
        return;
    }
    
    int itemHeight = 40;
    for (size_t i = 0; i < impl_->plugins.size(); ++i) {
        if (bounds.getHeight() < itemHeight) break;
        
        const auto& plugin = impl_->plugins[i];
        
        if (impl_->showSandboxedOnly && !plugin.sandboxed) continue;
        
        auto row = bounds.removeFromTop(itemHeight);
        bool isSelected = static_cast<int>(i) == impl_->selectedIndex;
        
        // Row background
        if (isSelected) {
            g.setColour(juce::Colour(0xff4080ff).withAlpha(0.2f));
            g.fillRect(row);
        }
        
        // Plugin name
        g.setColour(juce::Colours::white);
        auto nameCol = row.withWidth(200);
        g.drawText(plugin.name, nameCol.reduced(5, 0), juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff606060));
        g.drawText(plugin.vendor, nameCol.reduced(5, 0).translated(0, 12), 
                   juce::Justification::centredLeft);
        
        // Status indicator
        juce::Colour statusColor;
        juce::String statusText;
        switch (plugin.status) {
            case Impl::PluginInfo::Status::Healthy:
                statusColor = juce::Colour(0xff40ff40);
                statusText = "OK";
                break;
            case Impl::PluginInfo::Status::Warning:
                statusColor = juce::Colour(0xffffaa40);
                statusText = "Warn";
                break;
            case Impl::PluginInfo::Status::Error:
                statusColor = juce::Colour(0xffff4040);
                statusText = "Error";
                break;
            case Impl::PluginInfo::Status::Crashed:
                statusColor = juce::Colour(0xffff0000);
                statusText = "Crash";
                break;
            case Impl::PluginInfo::Status::Sandboxed:
                statusColor = juce::Colour(0xff4080ff);
                statusText = "Boxed";
                break;
        }
        
        auto statusCol = row.withX(200).withWidth(80);
        g.setColour(statusColor);
        g.fillRoundedRectangle(statusCol.reduced(10, 8), 3.0f);
        g.setColour(juce::Colours::black);
        g.drawText(statusText, statusCol, juce::Justification::centred);
        
        // CPU usage
        auto cpuCol = row.withX(280).withWidth(60);
        g.setColour(plugin.cpuUsage > 0.5f ? juce::Colour(0xffff4040) :
                    plugin.cpuUsage > 0.2f ? juce::Colour(0xffffaa40) :
                    juce::Colour(0xff40ff40));
        g.drawText(juce::String(plugin.cpuUsage * 100, 1) + "%", cpuCol, 
                   juce::Justification::centred);
        
        // Latency
        auto latCol = row.withX(340).withWidth(80);
        g.setColour(juce::Colour(0xff808080));
        g.drawText(juce::String(plugin.avgLatencyMs, 1) + "ms", latCol,
                   juce::Justification::centred);
        
        // Sandbox indicator
        auto sandboxCol = row.withX(420).withWidth(70);
        if (plugin.sandboxed) {
            g.setColour(juce::Colour(0xff4080ff));
            g.fillRoundedRectangle(sandboxCol.reduced(15, 10), 3.0f);
            g.setColour(juce::Colours::white);
            g.drawText("✓", sandboxCol, juce::Justification::centred);
        } else {
            g.setColour(juce::Colour(0xff404040));
            g.drawText("-", sandboxCol, juce::Justification::centred);
        }
    }
}

void PluginHealthWidget::resized() {
    // Layout handled in paint
}

int PluginHealthWidget::getPluginCount() const {
    return static_cast<int>(impl_->plugins.size());
}

void PluginHealthWidget::setSelectedIndex(int index) {
    impl_->selectedIndex = index;
    repaint();
}

int PluginHealthWidget::getSelectedIndex() const {
    return impl_->selectedIndex;
}

void PluginHealthWidget::setShowSandboxedOnly(bool show) {
    impl_->showSandboxedOnly = show;
    repaint();
}

bool PluginHealthWidget::isShowingSandboxedOnly() const {
    return impl_->showSandboxedOnly;
}

void PluginHealthWidget::refresh() {
    // TODO: Query plugin inspector for latest metrics
    repaint();
}

void PluginHealthWidget::sandboxSelected() {
    // TODO: Trigger sandbox for selected plugin
}

void PluginHealthWidget::suspendSelected() {
    // TODO: Suspend selected plugin
}

}  // namespace daw::ui::plugins

/**
 * @file PerfDashboard.cpp
 * @brief Performance monitoring dashboard (stub)
 */

#include "PerfDashboard.h"

namespace daw::ui::performance {

class PerfDashboard::Impl {
public:
    // CPU metrics
    float cpuUsage{0.0f};
    float audioThreadLoad{0.0f};
    float workerPoolLoad{0.0f};
    
    // Memory
    size_t usedMemoryMB{0};
    size_t totalMemoryMB{0};
    
    // Audio
    int bufferSize{512};
    double sampleRate{44100.0};
    float blockTimeMs{0.0f};
    float maxBlockTimeMs{0.0f};
    int dropouts{0};
    
    // Performance advisor
    int currentQualityTier{2};  // 0=Low, 1=Medium, 2=High, 3=Ultra
    bool adaptiveModeEnabled{true};
    
    // History for graphs
    std::vector<float> cpuHistory;
    std::vector<float> audioLoadHistory;
    static constexpr size_t historySize = 100;
};

PerfDashboard::PerfDashboard()
    : impl_(std::make_unique<Impl>()) {
    impl_->cpuHistory.resize(Impl::historySize, 0.0f);
    impl_->audioLoadHistory.resize(Impl::historySize, 0.0f);
}

PerfDashboard::~PerfDashboard() = default;

void PerfDashboard::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Title
    auto titleArea = bounds.removeFromTop(30);
    g.setColour(juce::Colours::white);
    g.drawText("Performance Dashboard", titleArea, juce::Justification::centredLeft);
    
    // Quality tier indicator
    juce::Colour tierColors[] = {
        juce::Colour(0xffff4040),  // Low - red
        juce::Colour(0xffffaa40),  // Medium - orange
        juce::Colour(0xff40ff40),  // High - green
        juce::Colour(0xff4080ff)   // Ultra - blue
    };
    juce::String tierNames[] = {"Low", "Medium", "High", "Ultra"};
    
    auto tierArea = titleArea.removeFromRight(100);
    g.setColour(tierColors[impl_->currentQualityTier]);
    g.fillRoundedRectangle(tierArea, 5.0f);
    g.setColour(juce::Colours::black);
    g.drawText(tierNames[impl_->currentQualityTier], tierArea, juce::Justification::centred);
    
    bounds.removeFromTop(10);
    
    // CPU section
    auto cpuSection = bounds.removeFromTop(80);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(cpuSection, 5.0f);
    
    g.setColour(juce::Colours::white);
    g.drawText("CPU", cpuSection.removeFromTop(20), juce::Justification::centredLeft);
    
    // CPU bar
    auto cpuBar = cpuSection.removeFromTop(25).reduced(5, 0);
    g.setColour(juce::Colour(0xff404040));
    g.fillRoundedRectangle(cpuBar, 3.0f);
    
    juce::Colour cpuColor = impl_->cpuUsage < 0.5f ? juce::Colour(0xff40ff40) :
                            impl_->cpuUsage < 0.75f ? juce::Colour(0xffffaa40) :
                            juce::Colour(0xffff4040);
    g.setColour(cpuColor);
    g.fillRoundedRectangle(cpuBar.withWidth(cpuBar.getWidth() * impl_->cpuUsage), 3.0f);
    
    g.setColour(juce::Colours::white);
    g.drawText(juce::String(impl_->cpuUsage * 100, 1) + "%", cpuBar, 
               juce::Justification::centred);
    
    // Audio load
    g.drawText("Audio Thread: " + juce::String(impl_->audioThreadLoad * 100, 1) + "%",
               cpuSection.removeFromTop(20), juce::Justification::centredLeft);
    
    bounds.removeFromTop(10);
    
    // Audio section
    auto audioSection = bounds.removeFromTop(100);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(audioSection, 5.0f);
    
    g.setColour(juce::Colours::white);
    g.drawText("Audio", audioSection.removeFromTop(20), juce::Justification::centredLeft);
    
    g.drawText("Buffer: " + juce::String(impl_->bufferSize) + " samples", 
               audioSection.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("Sample Rate: " + juce::String(impl_->sampleRate / 1000.0, 1) + " kHz",
               audioSection.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("Block Time: " + juce::String(impl_->blockTimeMs, 2) + " ms (max: " +
               juce::String(impl_->maxBlockTimeMs, 2) + " ms)",
               audioSection.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("Dropouts: " + juce::String(impl_->dropouts),
               audioSection.removeFromTop(20), juce::Justification::centredLeft);
    
    bounds.removeFromTop(10);
    
    // Memory section
    auto memSection = bounds.removeFromTop(50);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(memSection, 5.0f);
    
    g.setColour(juce::Colours::white);
    g.drawText("Memory: " + juce::String(impl_->usedMemoryMB) + " MB / " +
               juce::String(impl_->totalMemoryMB) + " MB",
               memSection, juce::Justification::centred);
    
    bounds.removeFromTop(10);
    
    // Graph area (stub)
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // Draw history graph
    if (!impl_->cpuHistory.empty()) {
        juce::Path path;
        float w = bounds.getWidth() / impl_->cpuHistory.size();
        
        for (size_t i = 0; i < impl_->cpuHistory.size(); ++i) {
            float x = bounds.getX() + i * w;
            float y = bounds.getBottom() - impl_->cpuHistory[i] * bounds.getHeight();
            
            if (i == 0) {
                path.startNewSubPath(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        
        g.setColour(juce::Colour(0xff4080ff));
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }
    
    g.setColour(juce::Colour(0xff808080));
    g.drawText("CPU History", bounds.removeFromTop(20), juce::Justification::centredLeft);
}

void PerfDashboard::resized() {
    // Layout handled in paint
}

void PerfDashboard::updateMetrics(float cpuUsage, float audioLoad, 
                                   size_t memUsedMB, size_t memTotalMB) {
    impl_->cpuUsage = cpuUsage;
    impl_->audioThreadLoad = audioLoad;
    impl_->usedMemoryMB = memUsedMB;
    impl_->totalMemoryMB = memTotalMB;
    
    // Update history
    impl_->cpuHistory.erase(impl_->cpuHistory.begin());
    impl_->cpuHistory.push_back(cpuUsage);
    
    repaint();
}

void PerfDashboard::setQualityTier(int tier) {
    impl_->currentQualityTier = juce::jlimit(0, 3, tier);
    repaint();
}

int PerfDashboard::getQualityTier() const {
    return impl_->currentQualityTier;
}

void PerfDashboard::setAdaptiveModeEnabled(bool enabled) {
    impl_->adaptiveModeEnabled = enabled;
}

bool PerfDashboard::isAdaptiveModeEnabled() const {
    return impl_->adaptiveModeEnabled;
}

void PerfDashboard::setAudioSettings(int bufferSize, double sampleRate) {
    impl_->bufferSize = bufferSize;
    impl_->sampleRate = sampleRate;
    repaint();
}

void PerfDashboard::reportDropout() {
    impl_->dropouts++;
    repaint();
}

void PerfDashboard::resetDropoutCount() {
    impl_->dropouts = 0;
    repaint();
}

}  // namespace daw::ui::performance

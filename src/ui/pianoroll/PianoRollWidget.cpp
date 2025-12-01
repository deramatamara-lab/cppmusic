/**
 * @file PianoRollWidget.cpp
 * @brief Spectral piano roll with harmonic overlay (stub)
 *
 * This widget provides a piano roll with spectral analysis overlay
 * and harmonic tension visualization.
 */

#include "PianoRollWidget.h"

namespace daw::ui::pianoroll {

class PianoRollWidget::Impl {
public:
    // Spectral display settings
    bool showSpectralOverlay{false};
    bool showHarmonicLane{false};
    float spectralOpacity{0.5f};
    
    // Note data (stub)
    struct Note {
        int pitch{60};
        double startTime{0.0};
        double duration{1.0};
        float velocity{0.8f};
    };
    std::vector<Note> notes;
    
    // Harmonic analysis results (stub)
    struct HarmonicInfo {
        double time{0.0};
        float tension{0.0f};
        juce::String chordLabel;
    };
    std::vector<HarmonicInfo> harmonicData;
    
    // View state
    double viewStartTime{0.0};
    double viewEndTime{8.0};
    int viewLowestNote{36};
    int viewHighestNote{96};
    float zoomLevel{1.0f};
};

PianoRollWidget::PianoRollWidget()
    : impl_(std::make_unique<Impl>()) {
}

PianoRollWidget::~PianoRollWidget() = default;

void PianoRollWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Grid lines
    g.setColour(juce::Colour(0xff2a2a2a));
    
    int numNotes = impl_->viewHighestNote - impl_->viewLowestNote;
    float noteHeight = bounds.getHeight() / numNotes;
    
    for (int i = 0; i <= numNotes; ++i) {
        float y = i * noteHeight;
        g.drawHorizontalLine(static_cast<int>(y), 0, bounds.getWidth());
    }
    
    // Time grid
    double viewDuration = impl_->viewEndTime - impl_->viewStartTime;
    int numBeats = static_cast<int>(viewDuration * 4);  // Assume 4 beats per second
    float beatWidth = bounds.getWidth() / numBeats;
    
    for (int i = 0; i <= numBeats; ++i) {
        float x = i * beatWidth;
        g.drawVerticalLine(static_cast<int>(x), 0, bounds.getHeight());
    }
    
    // Draw notes (stub)
    g.setColour(juce::Colour(0xff4080ff));
    for (const auto& note : impl_->notes) {
        float x = static_cast<float>((note.startTime - impl_->viewStartTime) / viewDuration * bounds.getWidth());
        float w = static_cast<float>(note.duration / viewDuration * bounds.getWidth());
        float y = (impl_->viewHighestNote - note.pitch) * noteHeight;
        
        g.fillRoundedRectangle(x, y, w, noteHeight * 0.9f, 3.0f);
    }
    
    // Spectral overlay (stub)
    if (impl_->showSpectralOverlay) {
        g.setColour(juce::Colour(0xffff8040).withAlpha(impl_->spectralOpacity));
        // TODO: Draw spectral data
        g.drawText("Spectral Overlay (stub)", bounds, juce::Justification::centred);
    }
    
    // Harmonic lane (stub)
    if (impl_->showHarmonicLane) {
        auto harmonicLane = bounds.removeFromBottom(40);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(harmonicLane);
        g.setColour(juce::Colour(0xffffffff));
        g.drawText("Harmonic Lane (stub)", harmonicLane, juce::Justification::centred);
    }
}

void PianoRollWidget::resized() {
    // Layout handled in paint
}

void PianoRollWidget::setShowSpectralOverlay(bool show) {
    impl_->showSpectralOverlay = show;
    repaint();
}

bool PianoRollWidget::isSpectralOverlayVisible() const {
    return impl_->showSpectralOverlay;
}

void PianoRollWidget::setShowHarmonicLane(bool show) {
    impl_->showHarmonicLane = show;
    repaint();
}

bool PianoRollWidget::isHarmonicLaneVisible() const {
    return impl_->showHarmonicLane;
}

void PianoRollWidget::setSpectralOpacity(float opacity) {
    impl_->spectralOpacity = juce::jlimit(0.0f, 1.0f, opacity);
    repaint();
}

float PianoRollWidget::getSpectralOpacity() const {
    return impl_->spectralOpacity;
}

void PianoRollWidget::setViewRange(double startTime, double endTime, 
                                    int lowestNote, int highestNote) {
    impl_->viewStartTime = startTime;
    impl_->viewEndTime = endTime;
    impl_->viewLowestNote = lowestNote;
    impl_->viewHighestNote = highestNote;
    repaint();
}

void PianoRollWidget::zoomToFit() {
    // TODO: Calculate bounds from notes
    repaint();
}

void PianoRollWidget::setZoomLevel(float zoom) {
    impl_->zoomLevel = juce::jlimit(0.1f, 10.0f, zoom);
    repaint();
}

float PianoRollWidget::getZoomLevel() const {
    return impl_->zoomLevel;
}

}  // namespace daw::ui::pianoroll

/**
 * @file PianoRollWidget.h
 * @brief Spectral piano roll widget header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::pianoroll {

/**
 * @brief Piano roll with spectral overlay and harmonic analysis
 *
 * Features:
 * - Standard MIDI note display
 * - Spectral overlay showing frequency content
 * - Harmonic tension lane
 * - Chord labeling
 */
class PianoRollWidget : public juce::Component {
public:
    PianoRollWidget();
    ~PianoRollWidget() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Spectral overlay
    void setShowSpectralOverlay(bool show);
    [[nodiscard]] bool isSpectralOverlayVisible() const;
    void setSpectralOpacity(float opacity);
    [[nodiscard]] float getSpectralOpacity() const;

    // Harmonic lane
    void setShowHarmonicLane(bool show);
    [[nodiscard]] bool isHarmonicLaneVisible() const;

    // View control
    void setViewRange(double startTime, double endTime, 
                      int lowestNote, int highestNote);
    void zoomToFit();
    void setZoomLevel(float zoom);
    [[nodiscard]] float getZoomLevel() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWidget)
};

}  // namespace daw::ui::pianoroll

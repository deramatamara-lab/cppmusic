#include "ArrangeView.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::views
{

ArrangeView::ArrangeView(std::shared_ptr<daw::project::ProjectModel> projectModel,
                         std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : projectModel(projectModel)
    , engineContext(engineContext)
    , pixelsPerBeat(50.0)
    , trackHeight(60)
    , rulerHeight(30)
    , isDragging(false)
    , draggedClipId(0)
    , dragStartBeats(0.0)
{
}

void ArrangeView::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    g.fillAll(juce::Colour(Colors::background));
    
    auto bounds = getLocalBounds();
    
    // Draw ruler with glassmorphism
    auto rulerBounds = bounds.removeFromTop(rulerHeight);
    drawRuler(g, rulerBounds);
    
    // Draw grid and tracks
    drawGrid(g, bounds);
    drawTracks(g, bounds);
    drawClips(g, bounds);
}

void ArrangeView::resized()
{
    refresh();
}

void ArrangeView::mouseDown(const juce::MouseEvent& e)
{
    if (projectModel == nullptr)
        return;
    
    auto clip = getClipAtPosition(e.position.toInt());
    if (clip != nullptr)
    {
        isDragging = true;
        draggedClipId = clip->getId();
        dragStartBeats = clip->getStartBeats();
        dragStartPos = e.position.toInt();
        
        // Select clip
        projectModel->getSelectionModel().clearAll();
        projectModel->getSelectionModel().selectClip(clip->getId());
    }
    else
    {
        // Deselect
        projectModel->getSelectionModel().clearAll();
    }
    
    repaint();
}

void ArrangeView::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || projectModel == nullptr)
        return;
    
    auto* clip = projectModel->getClip(draggedClipId);
    if (clip == nullptr)
        return;
    
    const auto deltaX = e.position.toInt().x - dragStartPos.x;
    const auto deltaBeats = beatsFromX(deltaX);
    const auto newStartBeats = dragStartBeats + deltaBeats;
    
    clip->setStartBeats(newStartBeats);
    repaint();
}

void ArrangeView::mouseUp(const juce::MouseEvent& /*e*/)
{
    isDragging = false;
}

void ArrangeView::refresh()
{
    repaint();
}

void ArrangeView::drawRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Enhanced glassmorphism background for ruler
    auto rulerBounds = bounds.toFloat();
    drawGlassPanel(g, rulerBounds, Radii::none, false);
    
    // Enhanced typography
    const auto font = getMonoFont(Typography::caption);
    g.setFont(font);
    
    const auto numBars = static_cast<int>((bounds.getWidth() / pixelsPerBeat) / 4.0) + 1;
    for (int bar = 0; bar < numBars; ++bar)
    {
        const auto x = xFromBeats(bar * 4.0);
        
        // Enhanced divider line with gradient
        juce::ColourGradient dividerGradient(juce::Colour(Colors::divider).withAlpha(0.3f),
                                            static_cast<float>(x),
                                            static_cast<float>(bounds.getY()),
                                            juce::Colour(Colors::divider),
                                            static_cast<float>(x),
                                            static_cast<float>(bounds.getBottom()),
                                            false);
        g.setGradientFill(dividerGradient);
        g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
        
        // Enhanced text with shadow
        const auto textBounds = juce::Rectangle<float>(static_cast<float>(x + 2), 
                                                       static_cast<float>(bounds.getY()), 
                                                       30.0f, 
                                                       static_cast<float>(bounds.getHeight()));
        drawTextWithShadow(g, juce::String(bar + 1), textBounds, juce::Justification::centredLeft, 
                          font, juce::Colour(Colors::textSoft), 1.0f, 0.2f);
    }
}

void ArrangeView::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Subtle grid lines with modern opacity
    g.setColour(juce::Colour(Colors::outline).withAlpha(0.15f));
    
    const auto numBeats = static_cast<int>((bounds.getWidth() / pixelsPerBeat)) + 1;
    for (int beat = 0; beat < numBeats; ++beat)
    {
        const auto x = xFromBeats(static_cast<double>(beat));
        g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
    }
}

void ArrangeView::drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    if (projectModel == nullptr)
        return;
    
    const auto tracks = projectModel->getTracks();
    auto y = bounds.getY();
    
    for (const auto* track : tracks)
    {
        if (!track->isVisible())
            continue;
        
        auto trackBounds = juce::Rectangle<int>(bounds.getX(), y, bounds.getWidth(), trackHeight);
        auto trackBoundsFloat = trackBounds.toFloat();
        
        // Glassmorphism for track header area (left side)
        auto headerBounds = trackBoundsFloat.removeFromLeft(120.0f);
        drawGlassPanel(g, headerBounds, Radii::none, false);
        
        // Track color accent
        g.setColour(track->getColor().withAlpha(0.15f));
        g.fillRect(trackBoundsFloat);
        
        // Divider line
        g.setColour(juce::Colour(Colors::divider));
        g.drawHorizontalLine(trackBounds.getBottom(), static_cast<float>(trackBounds.getX()), static_cast<float>(trackBounds.getRight()));
        
        // Enhanced track name with better typography and shadow
        const auto nameBounds = headerBounds.reduced(Spacing::small);
        const auto font = getBodyFont(Typography::body);
        drawTextWithShadow(g, track->getName(), nameBounds, juce::Justification::centredLeft,
                          font, juce::Colour(Colors::textSoft), 1.0f, 0.25f);
        
        y += trackHeight;
    }
}

void ArrangeView::drawClips(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    if (projectModel == nullptr)
        return;
    
    const auto& selection = projectModel->getSelectionModel();
    const auto clips = projectModel->getClips();
    
    for (const auto* clip : clips)
    {
        const auto* track = projectModel->getTrack(clip->getTrackId());
        if (track == nullptr || !track->isVisible())
            continue;
        
        const auto clipBounds = getClipBounds(clip);
        if (!bounds.intersects(clipBounds))
            continue;
        
        const auto isSelected = selection.isClipSelected(clip->getId());
        auto clipBoundsFloat = clipBounds.toFloat();
        
        // Shadow for depth
        if (isSelected)
        {
            applyShadow(g, Shadows::elevation2, clipBoundsFloat);
        }
        else
        {
            applyShadow(g, Shadows::elevation1, clipBoundsFloat);
        }
        
        // Clip background with gradient
        auto clipColor = isSelected ? track->getColor().brighter(0.2f) : track->getColor();
        juce::ColourGradient clipGradient(clipColor.brighter(0.1f),
                                         clipBoundsFloat.getX(),
                                         clipBoundsFloat.getY(),
                                         clipColor.darker(0.1f),
                                         clipBoundsFloat.getX(),
                                         clipBoundsFloat.getBottom(),
                                         false);
        g.setGradientFill(clipGradient);
        g.fillRoundedRectangle(clipBoundsFloat, Radii::small);
        
        // Border
        g.setColour(isSelected ? juce::Colour(Colors::outlineFocus) : clipColor.withAlpha(0.5f));
        g.drawRoundedRectangle(clipBoundsFloat, Radii::small, isSelected ? 2.0f : 1.0f);
        
        // Enhanced clip label with shadow
        const auto labelBounds = clipBounds.reduced(Spacing::xsmall).toFloat();
        const auto font = getBodyFont(Typography::caption);
        drawTextWithShadow(g, clip->getLabel(), labelBounds, juce::Justification::centred,
                          font, juce::Colour(Colors::textSoft), 1.0f, 0.3f);
    }
}

juce::Rectangle<int> ArrangeView::getClipBounds(const daw::project::Clip* clip) const
{
    if (projectModel == nullptr)
        return {};
    
    const auto* track = projectModel->getTrack(clip->getTrackId());
    if (track == nullptr)
        return {};
    
    const auto tracks = projectModel->getTracks();
    int trackIndex = 0;
    for (const auto* t : tracks)
    {
        if (t->getId() == track->getId())
            break;
        if (t->isVisible())
            ++trackIndex;
    }
    
    const auto x = xFromBeats(clip->getStartBeats());
    const auto width = static_cast<int>(clip->getLengthBeats() * pixelsPerBeat);
    const auto y = rulerHeight + trackIndex * trackHeight;
    
    return juce::Rectangle<int>(x, y, width, trackHeight);
}

daw::project::Clip* ArrangeView::getClipAtPosition(juce::Point<int> pos) const
{
    if (projectModel == nullptr)
        return nullptr;
    
    if (pos.y < rulerHeight)
        return nullptr;
    
    const auto clips = projectModel->getClips();
    for (auto* clip : clips)
    {
        const auto clipBounds = getClipBounds(clip);
        if (clipBounds.contains(pos))
            return clip;
    }
    
    return nullptr;
}

double ArrangeView::beatsFromX(int x) const
{
    return static_cast<double>(x) / pixelsPerBeat;
}

int ArrangeView::xFromBeats(double beats) const
{
    return static_cast<int>(beats * pixelsPerBeat);
}

} // namespace daw::ui::views


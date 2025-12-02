#include "PlaylistPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::imgui
{

PlaylistPanel::PlaylistPanel()
{
    // Initialize tracks using PlaylistTrack structure
    tracks_ = {
        {"Drums", {0.8f, 0.4f, 0.3f, 1.0f}, 60.0f, false, false, false, false, -1, false},
        {"Bass", {0.3f, 0.6f, 0.8f, 1.0f}, 60.0f, false, false, false, false, -1, false},
        {"Keys", {0.6f, 0.8f, 0.3f, 1.0f}, 60.0f, false, false, false, false, -1, false},
        {"Lead", {0.8f, 0.3f, 0.7f, 1.0f}, 60.0f, false, false, false, false, -1, false},
        {"Pad", {0.3f, 0.7f, 0.7f, 1.0f}, 60.0f, false, false, false, false, -1, false},
        {"FX", {0.7f, 0.5f, 0.3f, 1.0f}, 60.0f, false, false, false, false, -1, false}
    };
    createDemoContent();
}

void PlaylistPanel::createDemoContent()
{
    // Add some demo clips with deep-edit properties
    PatternClip clip1;
    clip1.name = "Kick Pattern";
    clip1.trackIndex = 0;
    clip1.startBeats = 0.0;
    clip1.lengthBeats = 8.0;
    clip1.color = ImVec4(0.8f, 0.4f, 0.3f, 1.0f);
    clips_.push_back(clip1);

    PatternClip clip2;
    clip2.name = "Kick Pattern";
    clip2.trackIndex = 0;
    clip2.startBeats = 16.0;
    clip2.lengthBeats = 8.0;
    clip2.color = ImVec4(0.8f, 0.4f, 0.3f, 1.0f);
    clips_.push_back(clip2);

    PatternClip clip3;
    clip3.name = "Bassline A";
    clip3.trackIndex = 1;
    clip3.startBeats = 0.0;
    clip3.lengthBeats = 16.0;
    clip3.color = ImVec4(0.3f, 0.5f, 0.8f, 1.0f);
    clip3.transpose = -12;  // Octave down
    clips_.push_back(clip3);

    PatternClip clip4;
    clip4.name = "Bassline B";
    clip4.trackIndex = 1;
    clip4.startBeats = 16.0;
    clip4.lengthBeats = 8.0;
    clip4.color = ImVec4(0.4f, 0.6f, 0.8f, 1.0f);
    clips_.push_back(clip4);

    PatternClip clip5;
    clip5.name = "Chord Prog";
    clip5.trackIndex = 2;
    clip5.startBeats = 0.0;
    clip5.lengthBeats = 16.0;
    clip5.color = ImVec4(0.5f, 0.8f, 0.4f, 1.0f);
    clips_.push_back(clip5);

    PatternClip clip6;
    clip6.name = "Chord Prog";
    clip6.trackIndex = 2;
    clip6.startBeats = 16.0;
    clip6.lengthBeats = 8.0;
    clip6.color = ImVec4(0.5f, 0.8f, 0.4f, 1.0f);
    clip6.stretchEnabled = true;
    clip6.stretchRatio = 1.5;  // Stretched
    clips_.push_back(clip6);

    PatternClip clip7;
    clip7.name = "Lead Melody";
    clip7.trackIndex = 3;
    clip7.startBeats = 8.0;
    clip7.lengthBeats = 8.0;
    clip7.color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
    clips_.push_back(clip7);

    PatternClip clip8;
    clip8.name = "Lead Hook";
    clip8.trackIndex = 3;
    clip8.startBeats = 16.0;
    clip8.lengthBeats = 8.0;
    clip8.color = ImVec4(0.9f, 0.6f, 0.2f, 1.0f);
    clip8.gain = 0.8f;  // Slightly quieter
    clips_.push_back(clip8);

    PatternClip clip9;
    clip9.name = "Pad Swell";
    clip9.trackIndex = 4;
    clip9.startBeats = 0.0;
    clip9.lengthBeats = 24.0;
    clip9.color = ImVec4(0.6f, 0.4f, 0.7f, 1.0f);
    clips_.push_back(clip9);
}

void PlaylistPanel::addClip(const PatternClip& clip)
{
    clips_.push_back(clip);
}

void PlaylistPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;

    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("Playlist", &open, ImGuiWindowFlags_MenuBar))
    {
        // Menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Make Unique", "Ctrl+U"))
                {
                    if (selectedClip_) makeClipUnique(selectedClip_);
                }
                if (ImGui::MenuItem("Consolidate", "Ctrl+J")) consolidateSelection();
                ImGui::Separator();
                if (ImGui::MenuItem("Slice at Playhead", "S"))
                {
                    if (selectedClip_) sliceClipAtPosition(selectedClip_, playheadPosition_);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Loop Region", nullptr, &loopEnabled_);
                ImGui::MenuItem("Follow Playhead", nullptr, &playheadFollowing_);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        float headerWidth = 120.0f * scale;
        float timelineHeight = 24.0f * scale;

        // Toolbar
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale));
        if (ImGui::BeginChild("##PlaylistToolbar", ImVec2(0, 32 * scale), true))
        {
            drawToolbar(theme);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        // Timeline header
        ImGui::Dummy(ImVec2(headerWidth, 0));
        ImGui::SameLine(0, 0);
        if (ImGui::BeginChild("##Timeline", ImVec2(0, timelineHeight), false))
        {
            drawTimeline(theme);
            if (loopEnabled_)
            {
                drawLoopRegion(theme);
            }
        }
        ImGui::EndChild();

        // Track headers + clips area
        float remainingHeight = contentSize.y - 32 * scale - timelineHeight;

        // Track headers
        if (ImGui::BeginChild("##TrackHeaders", ImVec2(headerWidth, remainingHeight), true))
        {
            for (size_t i = 0; i < tracks_.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));

                float y = static_cast<float>(i) * defaultTrackHeight_ * scale;
                ImGui::SetCursorPosY(y);

                ImGui::BeginGroup();
                ImGui::Text("%s", tracks_[i].name.c_str());

                // Small mute/solo buttons
                ImGui::SameLine();
                ImGui::SmallButton("M");
                ImGui::SameLine();
                ImGui::SmallButton("S");
                ImGui::EndGroup();

                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);        // Clips area
        if (ImGui::BeginChild("##ClipsArea", ImVec2(0, remainingHeight), false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
            drawTracks(theme);
            drawClips(theme);
            drawPlayhead(theme);
            drawSelectionMarquee(theme);
        }
        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void PlaylistPanel::drawToolbar(const Theme& theme)
{
    float scale = theme.getDpiScale();

    // Tool buttons
    const char* toolNames[] = {"Select", "Slice", "Slip", "Stretch", "Draw"};
    for (int i = 0; i < 5; ++i)
    {
        if (i > 0) ImGui::SameLine();

        bool isActive = (static_cast<int>(currentTool_) == i);
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, theme.getTokens().buttonActive);
        }

        if (ImGui::Button(toolNames[i]))
        {
            currentTool_ = static_cast<PlaylistTool>(i);
        }

        if (isActive)
        {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // Snap controls
    ImGui::Checkbox("Snap", &snapEnabled_);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(60 * scale);
    const char* snapItems[] = {"1", "1/2", "1/4", "1/8", "1/16"};
    int snapIdx = 2;
    switch (snapDivision_)
    {
        case 1: snapIdx = 0; break;
        case 2: snapIdx = 1; break;
        case 4: snapIdx = 2; break;
        case 8: snapIdx = 3; break;
        case 16: snapIdx = 4; break;
        default: break;
    }
    if (ImGui::Combo("##SnapDiv", &snapIdx, snapItems, 5))
    {
        int divisions[] = {1, 2, 4, 8, 16};
        snapDivision_ = divisions[snapIdx];
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // Loop toggle
    ImGui::Checkbox("Loop", &loopEnabled_);

    ImGui::SameLine();

    // Zoom controls
    float rightPadding = 150.0f * scale;
    ImGui::SameLine(ImGui::GetWindowWidth() - rightPadding);

    if (ImGui::Button("-##zoom")) { zoomX_ = std::max(0.25f, zoomX_ - 0.25f); }
    ImGui::SameLine();
    ImGui::Text("%.0f%%", zoomX_ * 100);
    ImGui::SameLine();
    if (ImGui::Button("+##zoom")) { zoomX_ = std::min(4.0f, zoomX_ + 0.25f); }
}

void PlaylistPanel::drawTimeline(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.menuBarBg);
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor);

    // Bar numbers and tick marks
    int numBeats = static_cast<int>(size.x / beatWidth) + 8;
    int startBeat = static_cast<int>(scrollX_);

    for (int i = 0; i < numBeats; ++i)
    {
        int beat = startBeat + i;
        float x = pos.x + static_cast<float>(i) * beatWidth - static_cast<float>(std::fmod(scrollX_, 1.0) * beatWidth);

        // Bar numbers (every 4 beats)
        if (beat % 4 == 0 && beat >= 0)
        {
            int bar = beat / 4 + 1;
            char label[16];
            std::snprintf(label, sizeof(label), "%d", bar);
            ImU32 textColor = ImGui::ColorConvertFloat4ToU32(tokens.text);
            drawList->AddText(ImVec2(x + 4, pos.y + 2), textColor, label);
        }

        // Tick marks
        float tickHeight = (beat % 4 == 0) ? size.y * 0.5f : size.y * 0.25f;
        ImU32 tickColor = ImGui::ColorConvertFloat4ToU32(
            (beat % 4 == 0) ? tokens.gridLineBar : tokens.gridLine
        );
        drawList->AddLine(
            ImVec2(x, pos.y + size.y - tickHeight),
            ImVec2(x, pos.y + size.y),
            tickColor
        );
    }
}

void PlaylistPanel::drawTracks(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    // Track backgrounds
    for (size_t i = 0; i < tracks_.size(); ++i)
    {
        float y = pos.y + static_cast<float>(i) * defaultTrackHeight_ * scale;

        ImVec4 trackColor = (i % 2 == 0) ? tokens.childBg :
            ImVec4(tokens.childBg.x * 1.1f, tokens.childBg.y * 1.1f, tokens.childBg.z * 1.1f, tokens.childBg.w);
        ImU32 color = ImGui::ColorConvertFloat4ToU32(trackColor);

        drawList->AddRectFilled(
            ImVec2(pos.x, y),
            ImVec2(pos.x + size.x, y + defaultTrackHeight_ * scale),
            color
        );
    }

    // Grid lines (beats)
    int numBeats = static_cast<int>(size.x / beatWidth) + 8;
    int startBeat = static_cast<int>(scrollX_);

    for (int i = 0; i < numBeats; ++i)
    {
        int beat = startBeat + i;
        float x = pos.x + static_cast<float>(i) * beatWidth - static_cast<float>(std::fmod(scrollX_, 1.0) * beatWidth);

        ImVec4 lineColor = (beat % 4 == 0) ? tokens.gridLineBar : tokens.gridLine;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(lineColor);

        drawList->AddLine(
            ImVec2(x, pos.y),
            ImVec2(x, pos.y + static_cast<float>(tracks_.size()) * defaultTrackHeight_ * scale),
            color,
            (beat % 4 == 0) ? 1.5f : 0.5f
        );
    }
}

void PlaylistPanel::drawClips(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    for (auto& clip : clips_)
    {
        float x = pos.x + static_cast<float>(clip.startBeats - scrollX_) * beatWidth;
        float y = pos.y + static_cast<float>(clip.trackIndex) * defaultTrackHeight_ * scale;
        float width = static_cast<float>(clip.lengthBeats) * beatWidth;
        float height = defaultTrackHeight_ * scale - 4.0f * scale;

        // Skip if off-screen
        if (x + width < pos.x || x > pos.x + ImGui::GetContentRegionAvail().x) continue;

        // Clip color
        ImVec4 clipColor = clip.muted ?
            ImVec4(clip.color.x * 0.5f, clip.color.y * 0.5f, clip.color.z * 0.5f, clip.color.w) :
            clip.color;

        if (clip.selected)
        {
            clipColor = ImVec4(
                std::min(clipColor.x * 1.2f, 1.0f),
                std::min(clipColor.y * 1.2f, 1.0f),
                std::min(clipColor.z * 1.2f, 1.0f),
                clipColor.w
            );
        }

        ImU32 color = ImGui::ColorConvertFloat4ToU32(clipColor);
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(
            clip.selected ? tokens.navHighlight : ImVec4(0, 0, 0, 0.3f)
        );

        // Draw clip
        drawList->AddRectFilled(
            ImVec2(x, y + 2 * scale),
            ImVec2(x + width - 1, y + height),
            color,
            tokens.radiusMd * scale
        );

        // Border
        drawList->AddRect(
            ImVec2(x, y + 2 * scale),
            ImVec2(x + width - 1, y + height),
            borderColor,
            tokens.radiusMd * scale,
            0,
            clip.selected ? 2.0f : 1.0f
        );

        // Clip name
        ImU32 textColor = ImGui::ColorConvertFloat4ToU32(tokens.text);
        ImVec2 textPos = ImVec2(x + 4 * scale, y + 4 * scale);

        // Clip text to bounds
        ImGui::PushClipRect(ImVec2(x, y), ImVec2(x + width, y + height), true);
        drawList->AddText(textPos, textColor, clip.name.c_str());
        ImGui::PopClipRect();
    }

    // Handle clip interaction
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
    {
        selectedClip_ = nullptr;
        for (auto& clip : clips_)
        {
            float x = pos.x + static_cast<float>(clip.startBeats - scrollX_) * beatWidth;
            float y = pos.y + static_cast<float>(clip.trackIndex) * defaultTrackHeight_ * scale;
            float width = static_cast<float>(clip.lengthBeats) * beatWidth;
            float height = defaultTrackHeight_ * scale;

            if (mousePos.x >= x && mousePos.x < x + width &&
                mousePos.y >= y && mousePos.y < y + height)
            {
                // Clear other selections
                for (auto& c : clips_) c.selected = false;
                clip.selected = true;
                selectedClip_ = &clip;
                if (onClipSelected_) onClipSelected_(&clip);
                break;
            }
        }
    }
}

void PlaylistPanel::drawPlayhead(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    // Simulate playhead position
    static double playheadPos = 0.0;
    playheadPos += ImGui::GetIO().DeltaTime * 2.0;  // 2 beats per second at 120 BPM
    if (playheadPos > 32.0) playheadPos = 0.0;

    float x = pos.x + static_cast<float>(playheadPos - scrollX_) * beatWidth;

    if (x >= pos.x && x <= pos.x + size.x)
    {
        ImU32 color = ImGui::ColorConvertFloat4ToU32(tokens.playhead);
        drawList->AddLine(
            ImVec2(x, pos.y),
            ImVec2(x, pos.y + static_cast<float>(tracks_.size()) * defaultTrackHeight_ * scale),
            color,
            2.0f
        );

        // Playhead triangle
        drawList->AddTriangleFilled(
            ImVec2(x - 6, pos.y),
            ImVec2(x + 6, pos.y),
            ImVec2(x, pos.y + 10),
            color
        );
    }
}

void PlaylistPanel::drawSelectionMarquee(const Theme& theme)
{
    if (!isSelecting_) return;

    const auto& tokens = theme.getTokens();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 min = ImVec2(
        std::min(selectionStart_.x, selectionEnd_.x),
        std::min(selectionStart_.y, selectionEnd_.y)
    );
    ImVec2 max = ImVec2(
        std::max(selectionStart_.x, selectionEnd_.x),
        std::max(selectionStart_.y, selectionEnd_.y)
    );

    ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(tokens.selection);
    ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(tokens.navHighlight);

    drawList->AddRectFilled(min, max, fillColor);
    drawList->AddRect(min, max, borderColor);
}

double PlaylistPanel::snapToGrid(double beats) const
{
    if (!snapEnabled_) return beats;
    double gridSize = 1.0 / snapDivision_;
    return std::round(beats / gridSize) * gridSize;
}

void PlaylistPanel::drawLoopRegion(const Theme& /*theme*/)
{
    float scale = 1.0f; // Use default scale

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    float loopStartX = pos.x + static_cast<float>(loopStart_ - scrollX_) * beatWidth;
    float loopEndX = pos.x + static_cast<float>(loopEnd_ - scrollX_) * beatWidth;

    // Loop region background
    ImU32 loopColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.6f, 0.9f, 0.2f));
    drawList->AddRectFilled(
        ImVec2(loopStartX, pos.y),
        ImVec2(loopEndX, pos.y + size.y),
        loopColor
    );

    // Loop markers
    ImU32 markerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    drawList->AddLine(ImVec2(loopStartX, pos.y), ImVec2(loopStartX, pos.y + size.y), markerColor, 2.0f);
    drawList->AddLine(ImVec2(loopEndX, pos.y), ImVec2(loopEndX, pos.y + size.y), markerColor, 2.0f);

    // Loop bracket indicators
    drawList->AddTriangleFilled(
        ImVec2(loopStartX, pos.y),
        ImVec2(loopStartX + 8, pos.y),
        ImVec2(loopStartX, pos.y + 8),
        markerColor
    );
    drawList->AddTriangleFilled(
        ImVec2(loopEndX - 8, pos.y),
        ImVec2(loopEndX, pos.y),
        ImVec2(loopEndX, pos.y + 8),
        markerColor
    );
}

void PlaylistPanel::drawClipWarpMarkers(const PatternClip& clip, const Theme& theme,
                                         ImVec2 clipPos, ImVec2 clipSize)
{
    if (clip.warpMarkers.empty()) return;

    float scale = theme.getDpiScale();
    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (const auto& marker : clip.warpMarkers)
    {
        float markerX = clipPos.x + static_cast<float>(marker.sourceBeat) * beatWidth;

        if (markerX < clipPos.x || markerX > clipPos.x + clipSize.x) continue;

        ImU32 markerColor = ImGui::ColorConvertFloat4ToU32(
            marker.selected ? ImVec4(1.0f, 0.5f, 0.2f, 1.0f) : ImVec4(0.9f, 0.6f, 0.2f, 0.8f)
        );

        // Warp marker triangle
        drawList->AddTriangleFilled(
            ImVec2(markerX - 4 * scale, clipPos.y),
            ImVec2(markerX + 4 * scale, clipPos.y),
            ImVec2(markerX, clipPos.y + 8 * scale),
            markerColor
        );

        // Vertical line
        drawList->AddLine(
            ImVec2(markerX, clipPos.y + 8 * scale),
            ImVec2(markerX, clipPos.y + clipSize.y),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.6f, 0.2f, 0.3f)),
            1.0f
        );
    }
}

void PlaylistPanel::sliceClipAtPosition(PatternClip* clip, double position)
{
    if (!clip) return;

    // Check if position is within clip bounds
    if (position <= clip->startBeats || position >= clip->startBeats + clip->lengthBeats)
        return;

    double relativePosition = position - clip->startBeats;

    // Create second clip (after slice point)
    PatternClip secondClip = *clip;
    secondClip.startBeats = position;
    secondClip.lengthBeats = clip->lengthBeats - relativePosition;
    secondClip.slipOffset = clip->slipOffset + relativePosition;
    secondClip.selected = false;

    // Modify original clip (before slice point)
    clip->lengthBeats = relativePosition;

    // Add the new clip
    clips_.push_back(secondClip);
}

void PlaylistPanel::makeClipUnique(PatternClip* clip)
{
    if (!clip) return;

    // Mark clip as unique (would typically create a copy of the pattern)
    // For now just append "(unique)" to the name
    clip->name += " (unique)";
}

void PlaylistPanel::consolidateSelection()
{
    // Find selected clips on the same track
    std::vector<PatternClip*> selectedOnTrack;
    int trackIndex = -1;
    double minStart = 999999.0;
    double maxEnd = 0.0;

    for (auto& clip : clips_)
    {
        if (clip.selected)
        {
            if (trackIndex == -1)
            {
                trackIndex = clip.trackIndex;
            }
            else if (clip.trackIndex != trackIndex)
            {
                // Selection spans multiple tracks - can't consolidate
                return;
            }

            selectedOnTrack.push_back(&clip);
            minStart = std::min(minStart, clip.startBeats);
            maxEnd = std::max(maxEnd, clip.startBeats + clip.lengthBeats);
        }
    }

    if (selectedOnTrack.size() < 2) return;

    // Create consolidated clip
    PatternClip consolidated;
    consolidated.name = "Consolidated";
    consolidated.trackIndex = trackIndex;
    consolidated.startBeats = minStart;
    consolidated.lengthBeats = maxEnd - minStart;
    consolidated.color = selectedOnTrack[0]->color;
    consolidated.selected = true;

    // Remove original clips
    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
            [](const PatternClip& c) { return c.selected; }),
        clips_.end()
    );

    // Add consolidated clip
    clips_.push_back(consolidated);
}

void PlaylistPanel::fitClipToTempo(PatternClip* clip, double targetBPM)
{
    if (!clip) return;

    // Calculate stretch ratio based on original and target tempo
    // Assuming original tempo is 120 BPM
    double originalBPM = 120.0;
    clip->stretchRatio = targetBPM / originalBPM;
    clip->stretchEnabled = true;

    // Adjust clip length based on stretch
    clip->lengthBeats *= clip->stretchRatio;
}

} // namespace daw::ui::imgui

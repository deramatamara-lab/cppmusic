#include "PlaylistPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::imgui
{

PlaylistPanel::PlaylistPanel()
{
    // Initialize tracks
    trackNames_ = {"Drums", "Bass", "Keys", "Lead", "Pad", "FX"};
    createDemoContent();
}

void PlaylistPanel::createDemoContent()
{
    // Add some demo clips
    clips_.push_back({"Kick Pattern", 0, 0.0, 8.0, ImVec4(0.8f, 0.4f, 0.3f, 1.0f), false, false});
    clips_.push_back({"Kick Pattern", 0, 16.0, 8.0, ImVec4(0.8f, 0.4f, 0.3f, 1.0f), false, false});
    
    clips_.push_back({"Bassline A", 1, 0.0, 16.0, ImVec4(0.3f, 0.5f, 0.8f, 1.0f), false, false});
    clips_.push_back({"Bassline B", 1, 16.0, 8.0, ImVec4(0.4f, 0.6f, 0.8f, 1.0f), false, false});
    
    clips_.push_back({"Chord Prog", 2, 0.0, 16.0, ImVec4(0.5f, 0.8f, 0.4f, 1.0f), false, false});
    clips_.push_back({"Chord Prog", 2, 16.0, 8.0, ImVec4(0.5f, 0.8f, 0.4f, 1.0f), false, false});
    
    clips_.push_back({"Lead Melody", 3, 8.0, 8.0, ImVec4(0.9f, 0.7f, 0.3f, 1.0f), false, false});
    clips_.push_back({"Lead Hook", 3, 16.0, 8.0, ImVec4(0.9f, 0.6f, 0.2f, 1.0f), false, false});
    
    clips_.push_back({"Pad Swell", 4, 0.0, 24.0, ImVec4(0.6f, 0.4f, 0.7f, 1.0f), false, false});
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
    
    if (ImGui::Begin("Playlist", &open))
    {
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        float headerWidth = 120.0f * scale;
        float timelineHeight = 24.0f * scale;
        
        // Toolbar
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale));
        if (ImGui::BeginChild("##PlaylistToolbar", ImVec2(0, 28 * scale), true))
        {
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
            
            // Zoom
            float rightPadding = 150.0f * scale;
            ImGui::SameLine(ImGui::GetWindowWidth() - rightPadding);
            
            if (ImGui::Button("-##zoom")) { zoomX_ = std::max(0.25f, zoomX_ - 0.25f); }
            ImGui::SameLine();
            ImGui::Text("%.0f%%", zoomX_ * 100);
            ImGui::SameLine();
            if (ImGui::Button("+##zoom")) { zoomX_ = std::min(4.0f, zoomX_ + 0.25f); }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        
        // Timeline header
        ImGui::Dummy(ImVec2(headerWidth, 0));
        ImGui::SameLine(0, 0);
        if (ImGui::BeginChild("##Timeline", ImVec2(0, timelineHeight), false))
        {
            drawTimeline(theme);
        }
        ImGui::EndChild();
        
        // Track headers + clips area
        float remainingHeight = contentSize.y - 28 * scale - timelineHeight;
        
        // Track headers
        if (ImGui::BeginChild("##TrackHeaders", ImVec2(headerWidth, remainingHeight), true))
        {
            for (size_t i = 0; i < trackNames_.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                
                float y = static_cast<float>(i) * trackHeight_ * scale;
                ImGui::SetCursorPosY(y);
                
                ImGui::BeginGroup();
                ImGui::Text("%s", trackNames_[i].c_str());
                
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
        
        ImGui::SameLine(0, 0);
        
        // Clips area
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
    for (size_t i = 0; i < trackNames_.size(); ++i)
    {
        float y = pos.y + static_cast<float>(i) * trackHeight_ * scale;
        
        ImVec4 trackColor = (i % 2 == 0) ? tokens.childBg : 
            ImVec4(tokens.childBg.x * 1.1f, tokens.childBg.y * 1.1f, tokens.childBg.z * 1.1f, tokens.childBg.w);
        ImU32 color = ImGui::ColorConvertFloat4ToU32(trackColor);
        
        drawList->AddRectFilled(
            ImVec2(pos.x, y),
            ImVec2(pos.x + size.x, y + trackHeight_ * scale),
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
            ImVec2(x, pos.y + static_cast<float>(trackNames_.size()) * trackHeight_ * scale),
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
        float y = pos.y + static_cast<float>(clip.trackIndex) * trackHeight_ * scale;
        float width = static_cast<float>(clip.lengthBeats) * beatWidth;
        float height = trackHeight_ * scale - 4.0f * scale;
        
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
            float y = pos.y + static_cast<float>(clip.trackIndex) * trackHeight_ * scale;
            float width = static_cast<float>(clip.lengthBeats) * beatWidth;
            float height = trackHeight_ * scale;
            
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
            ImVec2(x, pos.y + static_cast<float>(trackNames_.size()) * trackHeight_ * scale),
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

} // namespace daw::ui::imgui

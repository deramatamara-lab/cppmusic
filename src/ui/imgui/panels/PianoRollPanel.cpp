#include "PianoRollPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::imgui
{

PianoRollPanel::PianoRollPanel()
{
    createDemoNotes();
}

void PianoRollPanel::createDemoNotes()
{
    // Create a simple C major chord progression
    // C chord (C-E-G)
    notes_.push_back({60, 0.0, 2.0, 0.8f, false});  // C
    notes_.push_back({64, 0.0, 2.0, 0.7f, false});  // E
    notes_.push_back({67, 0.0, 2.0, 0.7f, false});  // G
    
    // F chord (F-A-C)
    notes_.push_back({65, 2.0, 2.0, 0.8f, false});  // F
    notes_.push_back({69, 2.0, 2.0, 0.7f, false});  // A
    notes_.push_back({72, 2.0, 2.0, 0.7f, false});  // C
    
    // G chord (G-B-D)
    notes_.push_back({67, 4.0, 2.0, 0.8f, false});  // G
    notes_.push_back({71, 4.0, 2.0, 0.7f, false});  // B
    notes_.push_back({74, 4.0, 2.0, 0.7f, false});  // D
    
    // C chord (C-E-G)
    notes_.push_back({60, 6.0, 2.0, 0.9f, false});  // C
    notes_.push_back({64, 6.0, 2.0, 0.8f, false});  // E
    notes_.push_back({67, 6.0, 2.0, 0.8f, false});  // G
}

void PianoRollPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;
    
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("Piano Roll", &open))
    {
        // Toolbar area
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale));
        if (ImGui::BeginChild("##PRToolbar", ImVec2(0, 32 * scale), true))
        {
            drawToolbar(theme);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        
        // Main content area
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        float keysWidth = 60.0f * scale;
        float velocityHeight = showVelocity_ ? 60.0f * scale : 0;
        
        // Piano keys column
        if (ImGui::BeginChild("##PianoKeys", ImVec2(keysWidth, contentSize.y - velocityHeight), false))
        {
            drawPianoKeys(theme);
        }
        ImGui::EndChild();
        
        ImGui::SameLine(0, 0);
        
        // Grid and notes area
        if (ImGui::BeginChild("##NoteGrid", ImVec2(0, contentSize.y - velocityHeight), false, 
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
            drawGrid(theme);
            drawNotes(theme);
        }
        ImGui::EndChild();
        
        // Velocity lane
        if (showVelocity_)
        {
            ImGui::Dummy(ImVec2(keysWidth, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::BeginChild("##VelocityLane", ImVec2(0, velocityHeight), true))
            {
                drawVelocityLane(theme);
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
}

void PianoRollPanel::drawToolbar(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    // Tool buttons
    const char* toolNames[] = {"Draw", "Select", "Erase"};
    for (int i = 0; i < 3; ++i)
    {
        if (i > 0) ImGui::SameLine();
        
        bool isActive = (static_cast<int>(currentTool_) == i);
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, tokens.buttonActive);
        }
        
        if (ImGui::Button(toolNames[i]))
        {
            currentTool_ = static_cast<Tool>(i);
        }
        
        if (isActive)
        {
            ImGui::PopStyleColor();
        }
    }
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Snap division
    ImGui::Text("Snap:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60 * scale);
    const char* snapItems[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32"};
    int snapIdx = 0;
    switch (snapDivision_)
    {
        case 1: snapIdx = 0; break;
        case 2: snapIdx = 1; break;
        case 4: snapIdx = 2; break;
        case 8: snapIdx = 3; break;
        case 16: snapIdx = 4; break;
        case 32: snapIdx = 5; break;
        default: break;
    }
    if (ImGui::Combo("##Snap", &snapIdx, snapItems, 6))
    {
        int divisions[] = {1, 2, 4, 8, 16, 32};
        snapDivision_ = divisions[snapIdx];
    }
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Scale lock
    ImGui::Checkbox("Scale Lock", &scaleLockEnabled_);
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Velocity lane toggle
    ImGui::Checkbox("Velocity", &showVelocity_);
    
    ImGui::SameLine();
    
    // Zoom controls
    float rightPadding = 150.0f * scale;
    ImGui::SameLine(ImGui::GetWindowWidth() - rightPadding);
    
    if (ImGui::Button("-")) { zoomX_ = std::max(0.25f, zoomX_ - 0.25f); }
    ImGui::SameLine();
    ImGui::Text("%.0f%%", zoomX_ * 100);
    ImGui::SameLine();
    if (ImGui::Button("+")) { zoomX_ = std::min(4.0f, zoomX_ + 0.25f); }
}

void PianoRollPanel::drawPianoKeys(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    float keyHeight = noteHeight_ * scale * zoomY_;
    int numKeys = static_cast<int>(size.y / keyHeight) + 2;
    int startPitch = static_cast<int>(scrollY_) - numKeys / 2;
    
    // Draw keys
    for (int i = 0; i < numKeys; ++i)
    {
        int pitch = startPitch + numKeys - i - 1;
        if (pitch < 0 || pitch > 127) continue;
        
        float y = pos.y + static_cast<float>(i) * keyHeight;
        bool isBlackKey = false;
        int noteInOctave = pitch % 12;
        if (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || 
            noteInOctave == 8 || noteInOctave == 10)
        {
            isBlackKey = true;
        }
        
        // Key color
        ImVec4 keyColor = isBlackKey ? ImVec4(0.2f, 0.2f, 0.2f, 1.0f) : ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        
        // Highlight if in scale (when scale lock enabled)
        if (scaleLockEnabled_ && !isNoteInScale(pitch))
        {
            keyColor = ImVec4(keyColor.x * 0.5f, keyColor.y * 0.5f, keyColor.z * 0.5f, keyColor.w);
        }
        
        ImU32 color = ImGui::ColorConvertFloat4ToU32(keyColor);
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(tokens.border);
        
        drawList->AddRectFilled(
            ImVec2(pos.x, y),
            ImVec2(pos.x + size.x, y + keyHeight),
            color
        );
        
        drawList->AddRect(
            ImVec2(pos.x, y),
            ImVec2(pos.x + size.x, y + keyHeight),
            borderColor
        );
        
        // Note name (for C notes)
        if (noteInOctave == 0)
        {
            int octave = pitch / 12 - 1;
            char label[8];
            std::snprintf(label, sizeof(label), "C%d", octave);
            ImU32 textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            drawList->AddText(ImVec2(pos.x + 4, y + 2), textColor, label);
        }
    }
}

void PianoRollPanel::drawGrid(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    float keyHeight = noteHeight_ * scale * zoomY_;
    float beatWidth = pixelsPerBeat_ * scale * zoomX_;
    
    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.childBg);
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor);
    
    // Horizontal lines (pitch)
    int numRows = static_cast<int>(size.y / keyHeight) + 2;
    int startPitch = static_cast<int>(scrollY_) - numRows / 2;
    
    for (int i = 0; i < numRows; ++i)
    {
        int pitch = startPitch + numRows - i - 1;
        float y = pos.y + static_cast<float>(i) * keyHeight;
        
        int noteInOctave = pitch % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || 
                           noteInOctave == 8 || noteInOctave == 10);
        
        ImVec4 rowColor = isBlackKey ? ImVec4(0.08f, 0.08f, 0.10f, 1.0f) : tokens.childBg;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(rowColor);
        
        drawList->AddRectFilled(
            ImVec2(pos.x, y),
            ImVec2(pos.x + size.x, y + keyHeight),
            color
        );
    }
    
    // Vertical lines (beats)
    int numBeats = static_cast<int>(size.x / beatWidth) + 2;
    int startBeat = static_cast<int>(scrollX_);
    
    for (int i = 0; i < numBeats; ++i)
    {
        int beat = startBeat + i;
        float x = pos.x + static_cast<float>(i) * beatWidth - static_cast<float>(std::fmod(scrollX_, 1.0) * beatWidth);
        
        ImVec4 lineColor = (beat % 4 == 0) ? tokens.gridLineBar :
                           (beat % 1 == 0) ? tokens.gridLineBeat : tokens.gridLine;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(lineColor);
        
        drawList->AddLine(
            ImVec2(x, pos.y),
            ImVec2(x, pos.y + size.y),
            color,
            (beat % 4 == 0) ? 2.0f : 1.0f
        );
        
        // Sub-divisions
        float subWidth = beatWidth / static_cast<float>(snapDivision_);
        for (int j = 1; j < snapDivision_; ++j)
        {
            float subX = x + static_cast<float>(j) * subWidth;
            ImU32 subColor = ImGui::ColorConvertFloat4ToU32(tokens.gridLine);
            drawList->AddLine(
                ImVec2(subX, pos.y),
                ImVec2(subX, pos.y + size.y),
                subColor
            );
        }
    }
}

void PianoRollPanel::drawNotes(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    float keyHeight = noteHeight_ * scale * zoomY_;
    float beatWidth = pixelsPerBeat_ * scale * zoomX_;
    
    int numRows = static_cast<int>(ImGui::GetContentRegionAvail().y / keyHeight) + 2;
    int centerPitch = static_cast<int>(scrollY_);
    
    for (auto& note : notes_)
    {
        // Calculate position
        int rowFromCenter = centerPitch - note.pitch;
        float y = pos.y + (static_cast<float>(numRows) / 2.0f + static_cast<float>(rowFromCenter)) * keyHeight;
        float x = pos.x + static_cast<float>(note.startBeats - scrollX_) * beatWidth;
        float width = static_cast<float>(note.lengthBeats) * beatWidth;
        
        // Skip if off-screen
        if (y + keyHeight < pos.y || y > pos.y + ImGui::GetContentRegionAvail().y) continue;
        if (x + width < pos.x || x > pos.x + ImGui::GetContentRegionAvail().x) continue;
        
        // Note color based on velocity and selection
        ImVec4 noteColor = note.selected ? tokens.selection : tokens.noteOn;
        noteColor.w = 0.5f + note.velocity * 0.5f;  // Velocity affects opacity
        
        ImU32 color = ImGui::ColorConvertFloat4ToU32(noteColor);
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(
            note.selected ? tokens.navHighlight : tokens.border
        );
        
        // Draw note rectangle
        drawList->AddRectFilled(
            ImVec2(x, y + 1),
            ImVec2(x + width - 1, y + keyHeight - 1),
            color,
            tokens.radiusSm * scale
        );
        
        drawList->AddRect(
            ImVec2(x, y + 1),
            ImVec2(x + width - 1, y + keyHeight - 1),
            borderColor,
            tokens.radiusSm * scale
        );
    }
}

void PianoRollPanel::drawVelocityLane(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    float beatWidth = pixelsPerBeat_ * scale * zoomX_;
    
    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.meterBackground);
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor);
    
    // Draw velocity bars for each note
    for (const auto& note : notes_)
    {
        float x = pos.x + static_cast<float>(note.startBeats - scrollX_) * beatWidth;
        float barWidth = std::max(4.0f * scale, static_cast<float>(note.lengthBeats) * beatWidth * 0.8f);
        float barHeight = note.velocity * (size.y - 4);
        
        // Skip if off-screen
        if (x + barWidth < pos.x || x > pos.x + size.x) continue;
        
        // Color based on velocity
        ImVec4 barColor = (note.velocity > 0.8f) ? tokens.meterRed :
                          (note.velocity > 0.5f) ? tokens.meterYellow : tokens.meterGreen;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(barColor);
        
        drawList->AddRectFilled(
            ImVec2(x + 2, pos.y + size.y - barHeight - 2),
            ImVec2(x + barWidth - 2, pos.y + size.y - 2),
            color,
            tokens.radiusSm * scale
        );
    }
}

bool PianoRollPanel::isNoteInScale(int pitch) const
{
    int noteInOctave = (pitch - scaleRoot_ + 12) % 12;
    return scaleNotes_[static_cast<size_t>(noteInOctave)];
}

double PianoRollPanel::snapToGrid(double beats) const
{
    double gridSize = 1.0 / snapDivision_;
    return std::round(beats / gridSize) * gridSize;
}

int PianoRollPanel::yToPitch(float y) const
{
    float keyHeight = noteHeight_ * zoomY_;
    return static_cast<int>(scrollY_) - static_cast<int>((y / keyHeight) - static_cast<float>(ImGui::GetContentRegionAvail().y / keyHeight) / 2);
}

float PianoRollPanel::pitchToY(int pitch) const
{
    float keyHeight = noteHeight_ * zoomY_;
    return (static_cast<float>(scrollY_ - pitch) + static_cast<float>(ImGui::GetContentRegionAvail().y / keyHeight) / 2) * keyHeight;
}

double PianoRollPanel::xToBeats(float x) const
{
    float beatWidth = pixelsPerBeat_ * zoomX_;
    return scrollX_ + static_cast<double>(x / beatWidth);
}

float PianoRollPanel::beatsToX(double beats) const
{
    float beatWidth = pixelsPerBeat_ * zoomX_;
    return static_cast<float>(beats - scrollX_) * beatWidth;
}

} // namespace daw::ui::imgui

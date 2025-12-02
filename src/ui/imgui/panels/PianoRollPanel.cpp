#include "PianoRollPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace daw::ui::imgui
{

PianoRollPanel::PianoRollPanel()
{
    createDemoNotes();
    updateUsedPitches();
}

void PianoRollPanel::createDemoNotes()
{
    // Create a simple C major chord progression with deep-edit features
    // C chord (C-E-G)
    notes_.push_back({60, 0.0, 2.0, 0.8f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});
    notes_.push_back({64, 0.0, 2.0, 0.7f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});
    notes_.push_back({67, 0.0, 2.0, 0.7f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});

    // F chord (F-A-C) with slide
    notes_.push_back({65, 2.0, 2.0, 0.8f, false, 0.5f, 0.0f, true, 0.25f, -2, 1.0f, 0, 1, 0, 0.0f});
    notes_.push_back({69, 2.0, 2.0, 0.7f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});
    notes_.push_back({72, 2.0, 2.0, 0.7f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});

    // G chord (G-B-D) with probability
    notes_.push_back({67, 4.0, 2.0, 0.8f, false, 0.5f, 0.0f, false, 0.0f, 0, 0.75f, 0, 1, 0, 0.0f});
    notes_.push_back({71, 4.0, 2.0, 0.7f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});
    notes_.push_back({74, 4.0, 2.0, 0.7f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 0, 0.0f});

    // C chord (C-E-G) with micro-timing
    notes_.push_back({60, 6.0, 2.0, 0.9f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 50, 0.0f});
    notes_.push_back({64, 6.0, 2.0, 0.8f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, -30, 0.0f});
    notes_.push_back({67, 6.0, 2.0, 0.8f, false, 0.5f, 0.0f, false, 0.0f, 0, 1.0f, 0, 1, 20, 0.0f});
}

void PianoRollPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;

    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("Piano Roll", &open, ImGuiWindowFlags_MenuBar))
    {
        // Menu bar with command palette access
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Select All", "Ctrl+A")) selectAll();
                if (ImGui::MenuItem("Delete", "Del")) deleteSelected();
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) duplicateSelected();
                ImGui::Separator();
                if (ImGui::MenuItem("Quantize", "Q")) quantizeSelected();
                if (ImGui::MenuItem("Legato", "L")) legato();
                ImGui::Separator();
                if (ImGui::MenuItem("Command Palette", "Ctrl+P")) showCommandPalette_ = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Velocity Lane", nullptr, &showVelocity_);
                ImGui::MenuItem("Probability Lane", nullptr, &showProbability_);
                ImGui::MenuItem("Micro-Timing Lane", nullptr, &showMicroTiming_);
                ImGui::Separator();
                ImGui::MenuItem("Ghost Notes", nullptr, &showGhostNotes_);
                ImGui::MenuItem("Scale Lock", nullptr, &scaleLockEnabled_);
                ImGui::MenuItem("Fold Mode", nullptr, &foldMode_);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Toolbar area
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale));
        if (ImGui::BeginChild("##PRToolbar", ImVec2(0, 36 * scale), true))
        {
            drawToolbar(theme);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        // Handle keyboard input
        handleInput(theme);

        // Main content area
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        float keysWidth = 60.0f * scale;
        float laneHeight = 0.0f;
        if (showVelocity_) laneHeight += 60.0f * scale;
        if (showProbability_) laneHeight += 40.0f * scale;
        if (showMicroTiming_) laneHeight += 40.0f * scale;

        // Piano keys column
        if (ImGui::BeginChild("##PianoKeys", ImVec2(keysWidth, contentSize.y - laneHeight), false))
        {
            drawPianoKeys(theme);
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        // Grid and notes area
        if (ImGui::BeginChild("##NoteGrid", ImVec2(0, contentSize.y - laneHeight), false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
            drawGrid(theme);
            if (showGhostNotes_) {
                // Draw ghost notes with reduced opacity
                // This would render notes from other patterns
            }
            drawNotes(theme);
            drawSlideConnections(theme);
            drawWarpMarkers(theme);
            drawHoverPreview(theme);
            drawBoxSelection(theme);

            // Handle pan/zoom
            handleZoomPan();
            handleToolInput(theme);
        }
        ImGui::EndChild();

        // Lanes
        if (showVelocity_)
        {
            ImGui::Dummy(ImVec2(keysWidth, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::BeginChild("##VelocityLane", ImVec2(0, 60.0f * scale), true))
            {
                drawVelocityLane(theme);
            }
            ImGui::EndChild();
        }

        if (showProbability_)
        {
            ImGui::Dummy(ImVec2(keysWidth, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::BeginChild("##ProbabilityLane", ImVec2(0, 40.0f * scale), true))
            {
                drawProbabilityLane(theme);
            }
            ImGui::EndChild();
        }

        if (showMicroTiming_)
        {
            ImGui::Dummy(ImVec2(keysWidth, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::BeginChild("##MicroTimingLane", ImVec2(0, 40.0f * scale), true))
            {
                drawMicroTimingLane(theme);
            }
            ImGui::EndChild();
        }

        // Command palette overlay
        if (showCommandPalette_)
        {
            drawCommandPalette(theme);
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void PianoRollPanel::drawToolbar(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    // Tool buttons - complete FL-style tool palette
    const char* toolNames[] = {"Draw", "Select", "Slice", "Glue", "Stretch", "Warp", "Erase", "Vel", "Nudge"};
    const int toolCount = 9;

    for (int i = 0; i < toolCount; ++i)
    {
        if (i > 0) ImGui::SameLine();

        bool isActive = (static_cast<int>(currentTool_) == i);
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, tokens.buttonActive);
        }

        if (ImGui::Button(toolNames[i]))
        {
            currentTool_ = static_cast<PianoRollTool>(i);
        }

        if (isActive)
        {
            ImGui::PopStyleColor();
        }

        // Tooltips for tools
        if (ImGui::IsItemHovered())
        {
            const char* tooltips[] = {
                "Draw notes (D)",
                "Select and move (V)",
                "Slice notes (S)",
                "Glue adjacent notes (G)",
                "Time-stretch selection",
                "Insert warp markers",
                "Erase notes (E)",
                "Paint velocity",
                "Nudge timing"
            };
            ImGui::SetTooltip("%s", tooltips[i]);
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

    // Scale lock toggle with scale selector
    ImGui::Checkbox("Scale", &scaleLockEnabled_);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale Lock - constrain notes to scale");

    if (scaleLockEnabled_)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40 * scale);
        const char* rootNotes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        if (ImGui::BeginCombo("##Root", rootNotes[scaleRoot_], ImGuiComboFlags_NoArrowButton))
        {
            for (int i = 0; i < 12; ++i)
            {
                if (ImGui::Selectable(rootNotes[i], scaleRoot_ == i))
                {
                    scaleRoot_ = i;
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // Ghost notes toggle
    ImGui::Checkbox("Ghost", &showGhostNotes_);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show ghost notes from other patterns");

    ImGui::SameLine();

    // Fold mode toggle
    ImGui::Checkbox("Fold", &foldMode_);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fold - show only used pitches");

    ImGui::SameLine();

    // Zoom controls
    float rightPadding = 150.0f * scale;
    ImGui::SameLine(ImGui::GetWindowWidth() - rightPadding);

    if (ImGui::Button("-##zoom")) { zoomX_ = std::max(0.25f, zoomX_ - 0.25f); }
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

    // Check for mouse click on keys
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseInKeysArea = (mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
                            mousePos.y >= pos.y && mousePos.y <= pos.y + size.y);

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

        // Check if this key is being clicked
        bool keyHovered = mouseInKeysArea &&
                          mousePos.y >= y && mousePos.y < y + keyHeight;
        bool keyPressed = keyHovered && ImGui::IsMouseClicked(0);

        // Key color - highlight when hovered/pressed
        ImVec4 keyColor;
        if (keyPressed)
        {
            keyColor = ImVec4(1.0f, 0.5f, 0.2f, 1.0f);  // FL-style orange when pressed
        }
        else if (keyHovered)
        {
            keyColor = isBlackKey ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f) : ImVec4(1.0f, 1.0f, 0.9f, 1.0f);
        }
        else
        {
            keyColor = isBlackKey ? ImVec4(0.2f, 0.2f, 0.2f, 1.0f) : ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        }

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

        // Play note preview when key is clicked
        if (keyPressed && previewOnClick_ && onNotePreview_)
        {
            onNotePreview_(pitch, 0.8f);
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

// =============================================================================
// New methods for deep-edit capabilities
// =============================================================================

void PianoRollPanel::drawSlideConnections(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    float keyHeight = noteHeight_ * scale * zoomY_;
    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    int numRows = static_cast<int>(ImGui::GetContentRegionAvail().y / keyHeight) + 2;
    int centerPitch = static_cast<int>(scrollY_);

    for (const auto& note : notes_)
    {
        if (!note.hasSlide) continue;

        // Calculate start position
        int rowFromCenter = centerPitch - note.pitch;
        float y1 = pos.y + (static_cast<float>(numRows) / 2.0f + static_cast<float>(rowFromCenter)) * keyHeight + keyHeight / 2;
        float x1 = pos.x + static_cast<float>(note.startBeats + note.lengthBeats - scrollX_) * beatWidth;

        // Calculate end position (slide target)
        int targetPitch = note.pitch + note.slideToPitch;
        int targetRowFromCenter = centerPitch - targetPitch;
        float y2 = pos.y + (static_cast<float>(numRows) / 2.0f + static_cast<float>(targetRowFromCenter)) * keyHeight + keyHeight / 2;
        float x2 = x1 + note.slideTime * beatWidth;

        // Draw slide curve
        ImU32 slideColor = ImGui::ColorConvertFloat4ToU32(ImVec4(tokens.noteOn.x, tokens.noteOn.y, tokens.noteOn.z, 0.7f));

        // Bezier curve for smooth slide visualization
        ImVec2 p1(x1, y1);
        ImVec2 p2(x1 + (x2 - x1) * 0.3f, y1);
        ImVec2 p3(x1 + (x2 - x1) * 0.7f, y2);
        ImVec2 p4(x2, y2);

        drawList->AddBezierCubic(p1, p2, p3, p4, slideColor, 2.0f * scale);

        // Draw slide target indicator
        drawList->AddCircleFilled(p4, 4.0f * scale, slideColor);
    }
}

void PianoRollPanel::drawProbabilityLane(const Theme& theme)
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

    // Draw probability diamonds for each note
    for (const auto& note : notes_)
    {
        float x = pos.x + static_cast<float>(note.startBeats - scrollX_) * beatWidth;
        float centerY = pos.y + size.y / 2;

        // Skip if off-screen
        if (x < pos.x - 20 || x > pos.x + size.x + 20) continue;

        // Diamond size based on probability
        float diamondSize = 6.0f * scale * note.probability;

        // Color based on probability (green = 100%, orange = 50%, red = low)
        ImVec4 probColor = note.probability > 0.8f ? tokens.meterGreen :
                           note.probability > 0.4f ? tokens.meterYellow : tokens.meterRed;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(probColor);

        // Draw diamond
        ImVec2 points[4] = {
            ImVec2(x, centerY - diamondSize),
            ImVec2(x + diamondSize, centerY),
            ImVec2(x, centerY + diamondSize),
            ImVec2(x - diamondSize, centerY)
        };
        drawList->AddConvexPolyFilled(points, 4, color);

        // Condition indicator
        if (note.condition != 0)
        {
            ImU32 condColor = ImGui::ColorConvertFloat4ToU32(tokens.text);
            char condLabel[8];
            switch (note.condition)
            {
                case 1: std::snprintf(condLabel, sizeof(condLabel), "1st"); break;
                case 2: std::snprintf(condLabel, sizeof(condLabel), "%dN", note.conditionParam); break;
                case 3: std::snprintf(condLabel, sizeof(condLabel), "/%d", note.conditionParam); break;
                case 4: std::snprintf(condLabel, sizeof(condLabel), "-%d", note.conditionParam); break;
                case 5: std::snprintf(condLabel, sizeof(condLabel), "?"); break;
                default: condLabel[0] = '\0';
            }
            drawList->AddText(ImVec2(x - 8, pos.y + 2), condColor, condLabel);
        }
    }
}

void PianoRollPanel::drawMicroTimingLane(const Theme& theme)
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

    // Center line (zero offset)
    float centerY = pos.y + size.y / 2;
    ImU32 centerLineColor = ImGui::ColorConvertFloat4ToU32(tokens.gridLine);
    drawList->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY), centerLineColor);

    // Draw micro-timing offset markers for each note
    for (const auto& note : notes_)
    {
        float x = pos.x + static_cast<float>(note.startBeats - scrollX_) * beatWidth;

        // Skip if off-screen
        if (x < pos.x - 20 || x > pos.x + size.x + 20) continue;

        // Normalize offset to -1.0 to 1.0 range (assuming ±500 samples as max)
        float normalizedOffset = std::clamp(static_cast<float>(note.microTimingOffset) / 500.0f, -1.0f, 1.0f);
        float offsetY = centerY - normalizedOffset * (size.y / 2 - 4);

        // Color based on offset direction
        ImVec4 offsetColor = normalizedOffset > 0 ? ImVec4(0.3f, 0.7f, 0.9f, 1.0f) :
                             normalizedOffset < 0 ? ImVec4(0.9f, 0.5f, 0.3f, 1.0f) :
                             tokens.text;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(offsetColor);

        // Draw offset marker
        drawList->AddCircleFilled(ImVec2(x, offsetY), 4.0f * scale, color);
        drawList->AddLine(ImVec2(x, centerY), ImVec2(x, offsetY), color, 1.5f * scale);
    }
}

void PianoRollPanel::drawWarpMarkers(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    if (warpMarkers_.empty()) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    // Draw warp markers as triangles at the top
    for (const auto& marker : warpMarkers_)
    {
        float x = pos.x + static_cast<float>(marker.sourceBeat - scrollX_) * beatWidth;

        if (x < pos.x - 20 || x > pos.x + size.x + 20) continue;

        ImU32 markerColor = ImGui::ColorConvertFloat4ToU32(
            marker.selected ? tokens.navHighlight : ImVec4(0.9f, 0.6f, 0.2f, 1.0f)
        );

        // Triangle marker
        drawList->AddTriangleFilled(
            ImVec2(x - 6 * scale, pos.y),
            ImVec2(x + 6 * scale, pos.y),
            ImVec2(x, pos.y + 10 * scale),
            markerColor
        );

        // Vertical line
        drawList->AddLine(
            ImVec2(x, pos.y + 10 * scale),
            ImVec2(x, pos.y + size.y),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.6f, 0.2f, 0.3f)),
            1.0f
        );
    }
}

void PianoRollPanel::drawHoverPreview(const Theme& theme)
{
    if (!showHoverPreview_) return;

    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    float keyHeight = noteHeight_ * scale * zoomY_;
    float beatWidth = pixelsPerBeat_ * scale * zoomX_;

    int numRows = static_cast<int>(ImGui::GetContentRegionAvail().y / keyHeight) + 2;
    int centerPitch = static_cast<int>(scrollY_);

    // Draw ghost preview of note being placed
    int rowFromCenter = centerPitch - hoverPreviewNote_.pitch;
    float y = pos.y + (static_cast<float>(numRows) / 2.0f + static_cast<float>(rowFromCenter)) * keyHeight;
    float x = pos.x + static_cast<float>(hoverPreviewNote_.startBeats - scrollX_) * beatWidth;
    float width = static_cast<float>(hoverPreviewNote_.lengthBeats) * beatWidth;

    // Semi-transparent ghost note
    ImVec4 ghostColor = tokens.noteOn;
    ghostColor.w = 0.3f;
    ImU32 color = ImGui::ColorConvertFloat4ToU32(ghostColor);

    drawList->AddRectFilled(
        ImVec2(x, y + 1),
        ImVec2(x + width - 1, y + keyHeight - 1),
        color,
        tokens.radiusSm * scale
    );
}

void PianoRollPanel::drawBoxSelection(const Theme& theme)
{
    if (!isBoxSelecting_) return;

    const auto& tokens = theme.getTokens();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 min(std::min(boxSelectStart_.x, boxSelectEnd_.x),
               std::min(boxSelectStart_.y, boxSelectEnd_.y));
    ImVec2 max(std::max(boxSelectStart_.x, boxSelectEnd_.x),
               std::max(boxSelectStart_.y, boxSelectEnd_.y));

    ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(tokens.selection.x, tokens.selection.y, tokens.selection.z, 0.2f));
    ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(tokens.navHighlight);

    drawList->AddRectFilled(min, max, fillColor);
    drawList->AddRect(min, max, borderColor);
}

void PianoRollPanel::drawCommandPalette(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    // Center the palette
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 paletteSize(400 * scale, 300 * scale);
    ImVec2 palettePos((windowSize.x - paletteSize.x) / 2, 50 * scale);

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + palettePos.x,
                                    ImGui::GetWindowPos().y + palettePos.y));
    ImGui::SetNextWindowSize(paletteSize);

    if (ImGui::Begin("##CommandPalette", &showCommandPalette_,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, tokens.frameBg);

        // Search input
        ImGui::SetNextItemWidth(-1);
        char inputBuf[256];
        std::strncpy(inputBuf, commandInput_.c_str(), sizeof(inputBuf) - 1);
        inputBuf[sizeof(inputBuf) - 1] = '\0';

        if (ImGui::InputText("##CmdInput", inputBuf, sizeof(inputBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            executeCommand(inputBuf);
            showCommandPalette_ = false;
            commandInput_.clear();
        }
        commandInput_ = inputBuf;

        ImGui::PopStyleColor();

        ImGui::Separator();

        // Command list
        const char* commands[] = {
            "Quantize - Snap notes to grid (Q)",
            "Legato - Extend notes to next note (L)",
            "Scale Lock Toggle - Constrain to scale",
            "Randomize Velocity - Add variation",
            "Randomize Timing - Humanize timing",
            "Make Unique - Detach from parent pattern",
            "Consolidate - Merge selection to single clip"
        };

        for (const char* cmd : commands)
        {
            if (commandInput_.empty() || std::string(cmd).find(commandInput_) != std::string::npos)
            {
                if (ImGui::Selectable(cmd))
                {
                    executeCommand(cmd);
                    showCommandPalette_ = false;
                    commandInput_.clear();
                }
            }
        }
    }
    ImGui::End();
}

void PianoRollPanel::handleInput(const Theme& /*theme*/)
{
    ImGuiIO& io = ImGui::GetIO();

    // Keyboard shortcuts
    if (!io.WantTextInput)
    {
        // Tool shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_D)) currentTool_ = PianoRollTool::Draw;
        if (ImGui::IsKeyPressed(ImGuiKey_V)) currentTool_ = PianoRollTool::Select;
        if (ImGui::IsKeyPressed(ImGuiKey_S)) currentTool_ = PianoRollTool::Slice;
        if (ImGui::IsKeyPressed(ImGuiKey_G)) currentTool_ = PianoRollTool::Glue;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) currentTool_ = PianoRollTool::Erase;

        // Edit shortcuts
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) selectAll();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) duplicateSelected();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_P)) showCommandPalette_ = true;
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) deleteSelected();
        if (ImGui::IsKeyPressed(ImGuiKey_Q)) quantizeSelected();
        if (ImGui::IsKeyPressed(ImGuiKey_L)) legato();

        // Escape to close palette
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            if (showCommandPalette_) showCommandPalette_ = false;
            else selectNone();
        }
    }

    // Update drag modifiers
    handleDragModifiers();
}

void PianoRollPanel::handleToolInput(const Theme& /*theme*/)
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // Check if mouse is in the grid area
    if (mousePos.x < winPos.x || mousePos.x > winPos.x + winSize.x ||
        mousePos.y < winPos.y || mousePos.y > winPos.y + winSize.y)
    {
        showHoverPreview_ = false;
        return;
    }

    float localX = mousePos.x - winPos.x;
    float localY = mousePos.y - winPos.y;

    double beat = xToBeats(localX);
    int pitch = yToPitch(localY);

    if (!dragIgnoreSnap_)
    {
        beat = snapToGrid(beat);
    }

    // Update hover preview for Draw tool
    if (currentTool_ == PianoRollTool::Draw)
    {
        showHoverPreview_ = true;
        hoverPreviewNote_.pitch = pitch;
        hoverPreviewNote_.startBeats = beat;
        hoverPreviewNote_.lengthBeats = 1.0 / snapDivision_;
    }
    else
    {
        showHoverPreview_ = false;
    }

    // Handle tool-specific clicks
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
    {
        switch (currentTool_)
        {
            case PianoRollTool::Draw:
            {
                // Create new note
                NoteEvent newNote;
                newNote.pitch = pitch;
                newNote.startBeats = beat;
                newNote.lengthBeats = 1.0 / snapDivision_;
                newNote.velocity = 0.8f;
                notes_.push_back(newNote);
                updateUsedPitches();

                if (onNoteChanged_) onNoteChanged_(newNote);

                // Preview the note (play sound)
                if (previewOnClick_ && onNotePreview_)
                {
                    onNotePreview_(pitch, newNote.velocity);
                }
                break;
            }

            case PianoRollTool::Select:
            {
                // Start box selection
                isBoxSelecting_ = true;
                boxSelectStart_ = mousePos;
                boxSelectEnd_ = mousePos;
                break;
            }

            case PianoRollTool::Erase:
            {
                // Erase note under cursor
                for (auto it = notes_.begin(); it != notes_.end(); ++it)
                {
                    if (it->pitch == pitch &&
                        beat >= it->startBeats &&
                        beat < it->startBeats + it->lengthBeats)
                    {
                        notes_.erase(it);
                        updateUsedPitches();
                        break;
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    // Update box selection
    if (isBoxSelecting_ && ImGui::IsMouseDown(0))
    {
        boxSelectEnd_ = mousePos;
    }

    // End box selection
    if (isBoxSelecting_ && ImGui::IsMouseReleased(0))
    {
        isBoxSelecting_ = false;
        // Select notes in box
        // (Implementation would check note positions against box bounds)
    }
}

void PianoRollPanel::handleDragModifiers()
{
    ImGuiIO& io = ImGui::GetIO();
    dragFineAdjust_ = io.KeyShift;    // Shift = fine adjust
    dragIgnoreSnap_ = io.KeyAlt;       // Alt = ignore snap
    dragDuplicating_ = io.KeyCtrl;     // Ctrl = duplicate while dragging
}

void PianoRollPanel::handleZoomPan()
{
    ImGuiIO& io = ImGui::GetIO();

    if (!ImGui::IsWindowHovered()) return;

    // Mouse wheel zoom (Ctrl + wheel)
    if (io.KeyCtrl && std::abs(io.MouseWheel) > 0.0f)
    {
        float zoomDelta = io.MouseWheel * 0.1f;
        zoomX_ = std::clamp(zoomX_ + zoomDelta, 0.25f, 4.0f);
    }
    // Vertical scroll (Shift + wheel for horizontal)
    else if (std::abs(io.MouseWheel) > 0.0f)
    {
        if (io.KeyShift)
        {
            scrollX_ -= io.MouseWheel * 2.0;
            scrollX_ = std::max(0.0, scrollX_);
        }
        else
        {
            scrollY_ += io.MouseWheel * 2.0;
            scrollY_ = std::clamp(scrollY_, 0.0, 127.0);
        }
    }

    // Middle mouse button pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        ImVec2 delta = io.MouseDelta;
        scrollX_ -= delta.x / (pixelsPerBeat_ * zoomX_);
        scrollY_ += delta.y / (noteHeight_ * zoomY_);

        scrollX_ = std::max(0.0, scrollX_);
        scrollY_ = std::clamp(scrollY_, 0.0, 127.0);
    }

    // Apply inertia (smooth zoom/pan)
    float inertiaDecay = 0.9f;
    scrollVelocityX_ *= inertiaDecay;
    scrollVelocityY_ *= inertiaDecay;
    zoomVelocity_ *= inertiaDecay;

    if (std::abs(scrollVelocityX_) > 0.01f) scrollX_ += scrollVelocityX_;
    if (std::abs(scrollVelocityY_) > 0.01f) scrollY_ += scrollVelocityY_;
    if (std::abs(zoomVelocity_) > 0.001f) zoomX_ = std::clamp(zoomX_ + zoomVelocity_, 0.25f, 4.0f);
}

void PianoRollPanel::selectAll()
{
    for (auto& note : notes_)
    {
        note.selected = true;
    }
}

void PianoRollPanel::selectNone()
{
    for (auto& note : notes_)
    {
        note.selected = false;
    }
}

void PianoRollPanel::deleteSelected()
{
    notes_.erase(
        std::remove_if(notes_.begin(), notes_.end(),
            [](const NoteEvent& n) { return n.selected; }),
        notes_.end()
    );
    updateUsedPitches();
}

void PianoRollPanel::duplicateSelected()
{
    std::vector<NoteEvent> newNotes;
    for (const auto& note : notes_)
    {
        if (note.selected)
        {
            NoteEvent copy = note;
            copy.startBeats += copy.lengthBeats; // Duplicate after original
            copy.selected = false;
            newNotes.push_back(copy);
        }
    }
    notes_.insert(notes_.end(), newNotes.begin(), newNotes.end());
    updateUsedPitches();
}

void PianoRollPanel::quantizeSelected()
{
    for (auto& note : notes_)
    {
        if (note.selected)
        {
            note.startBeats = snapToGrid(note.startBeats);
        }
    }
}

void PianoRollPanel::legato()
{
    // Sort notes by start time
    std::vector<NoteEvent*> sortedNotes;
    for (auto& note : notes_)
    {
        if (note.selected)
        {
            sortedNotes.push_back(&note);
        }
    }

    std::sort(sortedNotes.begin(), sortedNotes.end(),
        [](const NoteEvent* a, const NoteEvent* b) {
            return a->startBeats < b->startBeats;
        });

    // Extend each note to the next
    for (size_t i = 0; i < sortedNotes.size() - 1; ++i)
    {
        sortedNotes[i]->lengthBeats = sortedNotes[i + 1]->startBeats - sortedNotes[i]->startBeats;
    }
}

void PianoRollPanel::randomizeSelection(bool velocity, bool timing)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velDist(0.6f, 1.0f);
    std::uniform_int_distribution<int> timingDist(-50, 50);

    for (auto& note : notes_)
    {
        if (note.selected)
        {
            if (velocity)
            {
                note.velocity = velDist(gen);
            }
            if (timing)
            {
                note.microTimingOffset = timingDist(gen);
            }
        }
    }
}

void PianoRollPanel::makeUnique()
{
    // This would detach the pattern from any parent
    // For now, just a stub
}

void PianoRollPanel::executeCommand(const std::string& command)
{
    if (command.find("Quantize") != std::string::npos)
    {
        quantizeSelected();
    }
    else if (command.find("Legato") != std::string::npos)
    {
        legato();
    }
    else if (command.find("Scale Lock") != std::string::npos)
    {
        scaleLockEnabled_ = !scaleLockEnabled_;
    }
    else if (command.find("Randomize Velocity") != std::string::npos)
    {
        randomizeSelection(true, false);
    }
    else if (command.find("Randomize Timing") != std::string::npos)
    {
        randomizeSelection(false, true);
    }
    else if (command.find("Make Unique") != std::string::npos)
    {
        makeUnique();
    }
}

void PianoRollPanel::updateUsedPitches()
{
    usedPitches_.clear();
    for (const auto& note : notes_)
    {
        if (std::find(usedPitches_.begin(), usedPitches_.end(), note.pitch) == usedPitches_.end())
        {
            usedPitches_.push_back(note.pitch);
        }
    }
    std::sort(usedPitches_.begin(), usedPitches_.end());
}

} // namespace daw::ui::imgui

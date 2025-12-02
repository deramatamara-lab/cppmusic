#include "TransportBar.hpp"
#include "imgui.h"
#include <cmath>
#include <cstdio>

namespace daw::ui::imgui
{

TransportBar::TransportBar()
{
    // Initialize with default state
}

void TransportBar::draw(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    float height = 48.0f * scale;

    // Transport bar as a fixed-height child window at top
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale));

    if (ImGui::BeginChild("##TransportBar", ImVec2(0, height), true, ImGuiWindowFlags_NoScrollbar))
    {
        float buttonSize = 32.0f * scale;

        // Transport buttons
        ImGui::SetCursorPosY((height - buttonSize) * 0.5f - tokens.spacingXs * scale);

        drawPlayButton(theme);
        ImGui::SameLine();
        drawStopButton(theme);
        ImGui::SameLine();
        drawRecordButton(theme);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(tokens.spacingMd * scale, 0));
        ImGui::SameLine();

        // Song/Pattern Mode Selector (FL Style)
    drawModeSelector(theme);

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(tokens.spacingMd * scale, 0));
    ImGui::SameLine();

    // Pattern Selector
    drawPatternSelector(theme);

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(tokens.spacingMd * scale, 0));
    ImGui::SameLine();

    // BPM control
        ImGui::SetCursorPosY((height - 24.0f * scale) * 0.5f - tokens.spacingXs * scale);
        drawBpmControl(theme);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(tokens.spacingSm * scale, 0));
        ImGui::SameLine();

        // Time signature
        drawTimeSignature(theme);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(tokens.spacingMd * scale, 0));
        ImGui::SameLine();

        // Position display
        drawPositionDisplay(theme);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(tokens.spacingMd * scale, 0));
        ImGui::SameLine();

        // Metronome
        drawMetronome(theme);

        // CPU meter on right side
        float cpuMeterWidth = 100.0f * scale;
        ImGui::SameLine(ImGui::GetWindowWidth() - cpuMeterWidth - tokens.spacingMd * scale);
        ImGui::SetCursorPosY((height - 20.0f * scale) * 0.5f - tokens.spacingXs * scale);
        drawCpuMeter(theme);
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void TransportBar::drawPlayButton(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    float size = 32.0f * scale;

    ImVec4 buttonColor = state_.isPlaying ? tokens.playButton : tokens.button;
    ImVec4 hoverColor = ImVec4(
        std::min(buttonColor.x + 0.1f, 1.0f),
        std::min(buttonColor.y + 0.1f, 1.0f),
        std::min(buttonColor.z + 0.1f, 1.0f),
        buttonColor.w
    );

    ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor);

    const char* icon = state_.isPlaying ? "||" : ">";
    if (ImGui::Button(icon, ImVec2(size, size)))
    {
        state_.isPlaying = !state_.isPlaying;
        if (onPlay_) onPlay_(state_.isPlaying);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(state_.isPlaying ? "Pause (Space)" : "Play (Space)");
    }

    ImGui::PopStyleColor(3);
}

void TransportBar::drawModeSelector(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    // FL Studio style: "PAT" and "SONG" toggle
    // Orange when active

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    // PAT button
    bool isPat = (state_.mode == TransportMode::Pattern);
    ImVec4 patColor = isPat ? tokens.noteOn : tokens.button; // Orange if active
    ImVec4 patText = isPat ? ImVec4(0,0,0,1) : tokens.text;

    ImGui::PushStyleColor(ImGuiCol_Button, patColor);
    ImGui::PushStyleColor(ImGuiCol_Text, patText);

    if (ImGui::Button("PAT", ImVec2(40 * scale, 24 * scale)))
    {
        state_.mode = TransportMode::Pattern;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pattern Mode (L)");

    ImGui::PopStyleColor(2);
    ImGui::SameLine();

    // SONG button
    bool isSong = (state_.mode == TransportMode::Song);
    ImVec4 songColor = isSong ? tokens.noteOn : tokens.button; // Orange if active
    ImVec4 songText = isSong ? ImVec4(0,0,0,1) : tokens.text;

    ImGui::PushStyleColor(ImGuiCol_Button, songColor);
    ImGui::PushStyleColor(ImGuiCol_Text, songText);

    if (ImGui::Button("SONG", ImVec2(40 * scale, 24 * scale)))
    {
        state_.mode = TransportMode::Song;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Song Mode (L)");

    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar(2);
}

void TransportBar::drawPatternSelector(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushItemWidth(100.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tokens.spacingXs * scale, tokens.spacingXs * scale));

    // Simple pattern selector
    if (ImGui::BeginCombo("##Pattern", state_.patternName.c_str(), ImGuiComboFlags_NoArrowButton))
    {
        for (int i = 1; i <= 9; ++i)
        {
            std::string name = "Pattern " + std::to_string(i);
            bool isSelected = (state_.currentPattern == i);
            if (ImGui::Selectable(name.c_str(), isSelected))
            {
                state_.currentPattern = i;
                state_.patternName = name;
                if (onPatternChange_) onPatternChange_(i);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Scroll on hover to change pattern
    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            state_.currentPattern += (wheel > 0.0f ? 1 : -1);
            if (state_.currentPattern < 1) state_.currentPattern = 1;
            if (state_.currentPattern > 999) state_.currentPattern = 999;
            state_.patternName = "Pattern " + std::to_string(state_.currentPattern);
            if (onPatternChange_) onPatternChange_(state_.currentPattern);
        }
        ImGui::SetTooltip("Pattern Selector\nScroll to change");
    }

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
}

void TransportBar::drawStopButton(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    float size = 32.0f * scale;

    ImGui::PushStyleColor(ImGuiCol_Button, tokens.button);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens.stopButton);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, tokens.stopButton);

    if (ImGui::Button("[]", ImVec2(size, size)))
    {
        state_.isPlaying = false;
        state_.positionBeats = 0.0;
        if (onStop_) onStop_();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Stop (Enter)");
    }

    ImGui::PopStyleColor(3);
}

void TransportBar::drawRecordButton(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    float size = 32.0f * scale;

    ImVec4 buttonColor = state_.isRecording ? tokens.recordButton : tokens.button;

    ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens.recordButton);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, tokens.recordButton);

    // Blinking effect when recording
    if (state_.isRecording)
    {
        float blink = static_cast<float>(std::sin(ImGui::GetTime() * 4.0) * 0.5 + 0.5);
        ImVec4 blinkColor = ImVec4(
            buttonColor.x * (0.5f + 0.5f * blink),
            buttonColor.y * (0.5f + 0.5f * blink),
            buttonColor.z * (0.5f + 0.5f * blink),
            buttonColor.w
        );
        ImGui::PushStyleColor(ImGuiCol_Button, blinkColor);
    }

    if (ImGui::Button("O", ImVec2(size, size)))
    {
        state_.isRecording = !state_.isRecording;
        if (onRecord_) onRecord_(state_.isRecording);
    }

    if (state_.isRecording)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(state_.isRecording ? "Stop Recording (Ctrl+R)" : "Record (Ctrl+R)");
    }

    ImGui::PopStyleColor(3);
}

void TransportBar::drawBpmControl(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushItemWidth(80.0f * scale);

    float bpm = static_cast<float>(state_.bpm);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tokens.spacingXs * scale, tokens.spacingXs * scale));

    if (ImGui::DragFloat("##BPM", &bpm, 0.5f, 20.0f, 300.0f, "%.1f BPM"))
    {
        state_.bpm = static_cast<double>(bpm);
        if (onBpmChange_) onBpmChange_(state_.bpm);
    }

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Tempo (BPM)\nDrag or double-click to edit");
    }
}

void TransportBar::drawTimeSignature(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushItemWidth(50.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tokens.spacingXs * scale, tokens.spacingXs * scale));

    char timeSigBuf[16];
    std::snprintf(timeSigBuf, sizeof(timeSigBuf), "%d/%d", state_.beatsPerBar, state_.beatUnit);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, tokens.frameBg);
    ImGui::InputText("##TimeSig", timeSigBuf, sizeof(timeSigBuf), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Time Signature");
    }
}

void TransportBar::drawPositionDisplay([[maybe_unused]] const Theme& theme)
{
    const auto& tokens = theme.getTokens();

    // Time display
    std::string timeStr = formatTime(state_.positionBeats, state_.bpm, state_.beatsPerBar);
    std::string posStr = formatPosition(state_.positionBeats, state_.beatsPerBar);

    ImGui::PushStyleColor(ImGuiCol_Text, tokens.text);

    // Time (mm:ss.ms)
    ImGui::PushFont(nullptr);  // Would use monospace font here
    ImGui::Text("%s", timeStr.c_str());
    ImGui::PopFont();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Position (bar.beat.tick)
    ImGui::Text("%s", posStr.c_str());

    ImGui::PopStyleColor();
}

void TransportBar::drawMetronome(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    float size = 24.0f * scale;

    ImVec4 buttonColor = state_.metronomeEnabled ? tokens.buttonActive : tokens.button;

    ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens.buttonHovered);

    if (ImGui::Button("M", ImVec2(size, size)))
    {
        state_.metronomeEnabled = !state_.metronomeEnabled;
    }

    ImGui::PopStyleColor(2);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(state_.metronomeEnabled ? "Metronome: ON" : "Metronome: OFF");
    }
}

void TransportBar::drawCpuMeter(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    // Simulate CPU usage (in real implementation, get from audio engine)
    state_.cpuUsage = static_cast<float>(15.0 + 10.0 * std::sin(ImGui::GetTime() * 0.5));

    float width = 80.0f * scale;

    // Color based on usage
    ImVec4 meterColor = (state_.cpuUsage < 50.0f) ? tokens.meterGreen :
                        (state_.cpuUsage < 80.0f) ? tokens.meterYellow :
                                                     tokens.meterRed;

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, meterColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, tokens.meterBackground);

    char overlay[32];
    std::snprintf(overlay, sizeof(overlay), "CPU: %.0f%%", state_.cpuUsage);

    ImGui::ProgressBar(state_.cpuUsage / 100.0f, ImVec2(width, 0), overlay);

    ImGui::PopStyleColor(2);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Audio Engine CPU Usage");
    }
}

std::string TransportBar::formatTime(double beats, double bpm, int /*beatsPerBar*/)
{
    double seconds = beats * 60.0 / bpm;
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - std::floor(seconds)) * 1000);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d.%03d", minutes, secs, ms);
    return buf;
}

std::string TransportBar::formatPosition(double beats, int beatsPerBar)
{
    int bar = static_cast<int>(beats / beatsPerBar) + 1;
    int beat = static_cast<int>(std::fmod(beats, static_cast<double>(beatsPerBar))) + 1;
    int tick = static_cast<int>((beats - std::floor(beats)) * 960);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d.%d.%03d", bar, beat, tick);
    return buf;
}

} // namespace daw::ui::imgui

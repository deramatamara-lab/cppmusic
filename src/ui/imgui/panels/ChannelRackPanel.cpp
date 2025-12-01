#include "ChannelRackPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::imgui
{

ChannelRackPanel::ChannelRackPanel()
{
    createDemoChannels();
}

void ChannelRackPanel::createDemoChannels()
{
    // Create demo channels
    addChannel("Kick");
    addChannel("Snare");
    addChannel("Hi-Hat");
    addChannel("Bass");
    
    // Set some demo patterns
    if (channels_.size() >= 4)
    {
        // Kick on 1, 5, 9, 13
        channels_[0].steps[0] = true;
        channels_[0].steps[4] = true;
        channels_[0].steps[8] = true;
        channels_[0].steps[12] = true;
        
        // Snare on 5, 13
        channels_[1].steps[4] = true;
        channels_[1].steps[12] = true;
        
        // Hi-hat on all
        for (int i = 0; i < stepsPerPattern_; i += 2)
        {
            channels_[2].steps[static_cast<size_t>(i)] = true;
        }
        
        // Bass pattern
        channels_[3].steps[0] = true;
        channels_[3].steps[3] = true;
        channels_[3].steps[8] = true;
        channels_[3].steps[11] = true;
    }
}

void ChannelRackPanel::addChannel(const std::string& name)
{
    ChannelState channel;
    channel.name = name;
    channel.steps.resize(static_cast<size_t>(stepsPerPattern_), false);
    channel.velocities.resize(static_cast<size_t>(stepsPerPattern_), 0.8f);
    channels_.push_back(std::move(channel));
}

void ChannelRackPanel::setStepsPerPattern(int steps)
{
    stepsPerPattern_ = steps;
    for (auto& channel : channels_)
    {
        channel.steps.resize(static_cast<size_t>(steps), false);
        channel.velocities.resize(static_cast<size_t>(steps), 0.8f);
    }
}

void ChannelRackPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;
    
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingSm * scale));
    
    if (ImGui::Begin("Channel Rack", &open))
    {
        // Toolbar
        if (ImGui::Button(isDrawMode_ ? "Draw Mode" : "Select Mode"))
        {
            isDrawMode_ = !isDrawMode_;
        }
        ImGui::SameLine();
        
        ImGui::Checkbox("Velocity Lane", &showVelocityLane_);
        ImGui::SameLine();
        
        if (ImGui::Button("+"))
        {
            addChannel();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Channel");
        
        ImGui::Separator();
        
        // Channel list
        if (ImGui::BeginChild("##ChannelList", ImVec2(0, 0), false))
        {
            for (size_t i = 0; i < channels_.size(); ++i)
            {
                drawChannel(static_cast<int>(i), channels_[i], theme);
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
}

void ChannelRackPanel::drawChannel(int index, ChannelState& channel, const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushID(index);
    
    float rowHeight = showVelocityLane_ ? 60.0f * scale : 32.0f * scale;
    
    // Channel header (name, mute, solo)
    ImGui::BeginGroup();
    
    // Mute button
    ImVec4 muteColor = channel.muted ? ImVec4(0.8f, 0.3f, 0.3f, 1.0f) : tokens.button;
    ImGui::PushStyleColor(ImGuiCol_Button, muteColor);
    if (ImGui::Button("M", ImVec2(24 * scale, 24 * scale)))
    {
        channel.muted = !channel.muted;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mute");
    
    ImGui::SameLine();
    
    // Solo button
    ImVec4 soloColor = channel.soloed ? ImVec4(0.9f, 0.8f, 0.2f, 1.0f) : tokens.button;
    ImGui::PushStyleColor(ImGuiCol_Button, soloColor);
    if (ImGui::Button("S", ImVec2(24 * scale, 24 * scale)))
    {
        channel.soloed = !channel.soloed;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Solo");
    
    ImGui::SameLine();
    
    // Channel name
    ImGui::SetNextItemWidth(80.0f * scale);
    char nameBuf[64];
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", channel.name.c_str());
    if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
    {
        channel.name = nameBuf;
    }
    
    ImGui::SameLine();
    
    // Step grid
    drawStepGrid(index, channel, theme);
    
    ImGui::EndGroup();
    
    // Velocity lane (if enabled and channel is selected)
    if (showVelocityLane_ && index == selectedChannel_)
    {
        ImGui::Indent(110.0f * scale);
        drawVelocityLane(index, channel, theme);
        ImGui::Unindent(110.0f * scale);
    }
    
    ImGui::PopID();
}

void ChannelRackPanel::drawStepGrid(int channelIndex, ChannelState& channel, const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    float stepSize = 20.0f * scale;
    float stepSpacing = 2.0f * scale;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        float y = cursorPos.y;
        
        // Highlight every 4th step (beat)
        ImVec4 bgColor = (i % 4 == 0) ? tokens.frameBgHovered : tokens.frameBg;
        
        // Active step
        ImVec4 stepColor = channel.steps[static_cast<size_t>(i)] ? tokens.noteOn : bgColor;
        
        // Current playhead position
        if (i == currentStep_)
        {
            stepColor = ImVec4(
                stepColor.x * 1.3f,
                stepColor.y * 1.3f,
                stepColor.z * 1.3f,
                stepColor.w
            );
        }
        
        ImU32 color = ImGui::ColorConvertFloat4ToU32(stepColor);
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(tokens.border);
        
        drawList->AddRectFilled(
            ImVec2(x, y),
            ImVec2(x + stepSize, y + stepSize),
            color,
            tokens.radiusSm * scale
        );
        
        drawList->AddRect(
            ImVec2(x, y),
            ImVec2(x + stepSize, y + stepSize),
            borderColor,
            tokens.radiusSm * scale
        );
        
        // Velocity indicator (height)
        if (channel.steps[static_cast<size_t>(i)])
        {
            float velocity = channel.velocities[static_cast<size_t>(i)];
            float velHeight = stepSize * velocity * 0.8f;
            ImU32 velColor = ImGui::ColorConvertFloat4ToU32(tokens.meterGreen);
            drawList->AddRectFilled(
                ImVec2(x + 2, y + stepSize - velHeight - 2),
                ImVec2(x + stepSize - 2, y + stepSize - 2),
                velColor,
                tokens.radiusSm * scale / 2
            );
        }
    }
    
    // Invisible buttons for interaction
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        float y = cursorPos.y;
        
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::InvisibleButton("##step", ImVec2(stepSize, stepSize));
        
        if (ImGui::IsItemClicked(0))
        {
            if (isDrawMode_)
            {
                channel.steps[static_cast<size_t>(i)] = !channel.steps[static_cast<size_t>(i)];
                if (onStepChanged_)
                {
                    onStepChanged_(channelIndex, i, channel.steps[static_cast<size_t>(i)]);
                }
            }
            else
            {
                selectedChannel_ = channelIndex;
            }
        }
        
        // Draw on drag
        if (isDrawMode_ && ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
        {
            if (!channel.steps[static_cast<size_t>(i)])
            {
                channel.steps[static_cast<size_t>(i)] = true;
                if (onStepChanged_)
                {
                    onStepChanged_(channelIndex, i, true);
                }
            }
        }
    }
    
    // Reset cursor
    ImGui::SetCursorScreenPos(ImVec2(
        cursorPos.x + static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing),
        cursorPos.y + stepSize + stepSpacing
    ));
}

void ChannelRackPanel::drawVelocityLane(int /*channelIndex*/, ChannelState& channel, const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    float stepSize = 20.0f * scale;
    float stepSpacing = 2.0f * scale;
    float laneHeight = 40.0f * scale;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.frameBg);
    drawList->AddRectFilled(
        cursorPos,
        ImVec2(cursorPos.x + static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing), cursorPos.y + laneHeight),
        bgColor
    );
    
    // Velocity bars
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        if (!channel.steps[static_cast<size_t>(i)]) continue;
        
        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        float velocity = channel.velocities[static_cast<size_t>(i)];
        float barHeight = velocity * (laneHeight - 4);
        
        ImVec4 barColor = (velocity > 0.8f) ? tokens.meterRed :
                          (velocity > 0.5f) ? tokens.meterYellow : tokens.meterGreen;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(barColor);
        
        drawList->AddRectFilled(
            ImVec2(x + 2, cursorPos.y + laneHeight - barHeight - 2),
            ImVec2(x + stepSize - 2, cursorPos.y + laneHeight - 2),
            color,
            tokens.radiusSm * scale / 2
        );
    }
    
    // Interaction
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        if (!channel.steps[static_cast<size_t>(i)]) continue;
        
        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        ImGui::SetCursorScreenPos(ImVec2(x, cursorPos.y));
        ImGui::InvisibleButton("##vel", ImVec2(stepSize, laneHeight));
        
        if (ImGui::IsItemActive())
        {
            float mouseY = ImGui::GetIO().MousePos.y;
            float relY = 1.0f - (mouseY - cursorPos.y) / laneHeight;
            channel.velocities[static_cast<size_t>(i)] = std::clamp(relY, 0.0f, 1.0f);
        }
    }
    
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + laneHeight + stepSpacing));
}

} // namespace daw::ui::imgui

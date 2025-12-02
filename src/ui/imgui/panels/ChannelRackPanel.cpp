#include "ChannelRackPanel.hpp"
#include "BrowserPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace daw::ui::imgui
{

ChannelRackPanel::ChannelRackPanel()
{
    // Don't create demo channels - let them sync from audio engine
    // createDemoChannels();
}

void ChannelRackPanel::createDemoChannels()
{
    // Create demo channels - only called if no audio engine
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

        // Hi-hat on every other step
        for (int i = 0; i < stepsPerPattern_; i += 2)
        {
            channels_[2].steps[static_cast<size_t>(i)] = true;
            channels_[2].velocities[static_cast<size_t>(i)] = (i % 4 == 0) ? 1.0f : 0.6f;
        }

        // Bass pattern
        channels_[3].steps[0] = true;
        channels_[3].steps[3] = true;
        channels_[3].steps[8] = true;
        channels_[3].steps[11] = true;
    }
}

void ChannelRackPanel::addChannel(const std::string& name, ChannelType type)
{
    ChannelState channel;
    channel.name = name;
    channel.type = type;
    channel.steps.resize(static_cast<size_t>(stepsPerPattern_), false);
    channel.velocities.resize(static_cast<size_t>(stepsPerPattern_), 0.8f);
    channel.probabilities.resize(static_cast<size_t>(stepsPerPattern_), 1.0f);
    channel.conditions.resize(static_cast<size_t>(stepsPerPattern_), StepCondition::Always);
    channel.conditionParams.resize(static_cast<size_t>(stepsPerPattern_), 1);
    channel.microTimingOffsets.resize(static_cast<size_t>(stepsPerPattern_), 0);
    channels_.push_back(std::move(channel));
}

void ChannelRackPanel::loadSample(int channelIndex, const std::string& path)
{
    if (channelIndex >= 0 && channelIndex < static_cast<int>(channels_.size()))
    {
        auto& channel = channels_[channelIndex];
        channel.type = ChannelType::Sampler;
        channel.samplePath = path;

        // Update name to filename
        std::filesystem::path p(path);
        channel.name = p.stem().string();

        // TODO: Actually load audio data
    }
}

void ChannelRackPanel::setStepsPerPattern(int steps)
{
    stepsPerPattern_ = steps;
    for (auto& channel : channels_)
    {
        channel.steps.resize(static_cast<size_t>(steps), false);
        channel.velocities.resize(static_cast<size_t>(steps), 0.8f);
        channel.probabilities.resize(static_cast<size_t>(steps), 1.0f);
        channel.conditions.resize(static_cast<size_t>(steps), StepCondition::Always);
        channel.conditionParams.resize(static_cast<size_t>(steps), 1);
        channel.microTimingOffsets.resize(static_cast<size_t>(steps), 0);
    }
}

void ChannelRackPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;

    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingSm * scale));

    if (ImGui::Begin("Channel Rack", &open, ImGuiWindowFlags_MenuBar))
    {
        // Menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Velocity Lane", nullptr, &showVelocityLane_);
                ImGui::MenuItem("Probability Lane", nullptr, &showProbabilityLane_);
                ImGui::MenuItem("Condition Lane", nullptr, &showConditionLane_);
                ImGui::MenuItem("Micro-Timing Lane", nullptr, &showMicroTimingLane_);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Generate Flam...")) { /* TODO */ }
                if (ImGui::MenuItem("Generate Roll...")) { /* TODO */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Fill Pattern")) { /* TODO */ }
                if (ImGui::MenuItem("Clear Pattern"))
                {
                    for (auto& channel : channels_)
                    {
                        std::fill(channel.steps.begin(), channel.steps.end(), false);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Toolbar
        if (ImGui::Button(isDrawMode_ ? "Draw Mode" : "Select Mode"))
        {
            isDrawMode_ = !isDrawMode_;
        }
        ImGui::SameLine();

        // Lane toggles
        ImGui::Checkbox("Velocity", &showVelocityLane_);
        ImGui::SameLine();
        ImGui::Checkbox("Prob", &showProbabilityLane_);
        ImGui::SameLine();
        ImGui::Checkbox("Cond", &showConditionLane_);
        ImGui::SameLine();

        // Pattern swing control
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        ImGui::Text("Swing:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100 * scale);
        ImGui::SliderFloat("##PatSwing", &patternSwing_, -1.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pattern-level swing");

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

    // Channel name button (FL-style)
    ImGui::PushStyleColor(ImGuiCol_Button, channel.color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(channel.color.x * 1.2f, channel.color.y * 1.2f, channel.color.z * 1.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(channel.color.x * 0.8f, channel.color.y * 0.8f, channel.color.z * 0.8f, 1.0f));

    if (ImGui::Button(channel.name.c_str(), ImVec2(100.0f * scale, 24.0f * scale)))
    {
        selectedChannel_ = index;
        if (onChannelSelected_) onChannelSelected_(index);
    }

    // Double click to open plugin
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (onChannelDoubleClick_) onChannelDoubleClick_(index);
    }

    // Drag and drop target (for loading samples/presets)
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BROWSER_ITEM"))
        {
            const BrowserItem* item = *(const BrowserItem**)payload->Data;
            if (item && (item->type == BrowserItemType::AudioFile || item->type == BrowserItemType::Preset))
            {
                loadSample(index, item->path);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Step grid
    drawStepGrid(index, channel, theme);

    // Channel params button
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(24 * scale, 24 * scale)))
    {
        ImGui::OpenPopup("ChannelParams");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Channel Parameters");

    // Channel params popup
    if (ImGui::BeginPopup("ChannelParams"))
    {
        drawChannelParams(index, channel, theme);
        ImGui::EndPopup();
    }

    ImGui::EndGroup();

    // Additional lanes (when channel is selected)
    if (index == selectedChannel_)
    {
        float indent = 110.0f * scale;

        if (showVelocityLane_)
        {
            ImGui::Indent(indent);
            drawVelocityLane(index, channel, theme);
            ImGui::Unindent(indent);
        }

        if (showProbabilityLane_)
        {
            ImGui::Indent(indent);
            drawProbabilityLane(index, channel, theme);
            ImGui::Unindent(indent);
        }

        if (showConditionLane_)
        {
            ImGui::Indent(indent);
            drawConditionIndicators(index, channel, theme);
            ImGui::Unindent(indent);
        }
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

        // Highlight every 4th step (beat) - FL Studio style with alternating shade
        bool isBeat = (i % 4 == 0);
        bool isBar = (i % 16 == 0);
        ImVec4 bgColor = isBar ? ImVec4(0.25f, 0.25f, 0.28f, 1.0f) :
                         isBeat ? ImVec4(0.22f, 0.22f, 0.25f, 1.0f) :
                                  tokens.frameBg;

        // Active step - FL orange
        ImVec4 stepColor = channel.steps[static_cast<size_t>(i)] ?
            ImVec4(0.95f, 0.55f, 0.15f, 1.0f) : bgColor;

        // Current playhead position - bright white outline
        bool isCurrentStep = (i == currentStep_);

        ImU32 color = ImGui::ColorConvertFloat4ToU32(stepColor);
        ImU32 borderColor = isCurrentStep ?
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)) :
            ImGui::ColorConvertFloat4ToU32(tokens.border);

        drawList->AddRectFilled(
            ImVec2(x, y),
            ImVec2(x + stepSize, y + stepSize),
            color,
            tokens.radiusSm * scale
        );

        // Playhead indicator - thicker border for current step
        float borderThickness = isCurrentStep ? 2.0f : 1.0f;
        drawList->AddRect(
            ImVec2(x, y),
            ImVec2(x + stepSize, y + stepSize),
            borderColor,
            tokens.radiusSm * scale,
            0,
            borderThickness
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
        ImGui::PushID(i); // Unique ID per step to avoid ID conflicts
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
        ImGui::PopID(); // Restore previous ID
    }

    // Submit a dummy item to properly grow parent bounds
    float totalWidth = static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing);
    ImGui::Dummy(ImVec2(totalWidth, stepSize));
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

    // Submit a dummy item to properly grow parent bounds
    float totalWidth = static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing);
    ImGui::Dummy(ImVec2(totalWidth, laneHeight));
}

void ChannelRackPanel::drawProbabilityLane(int /*channelIndex*/, ChannelState& channel, const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    float stepSize = 20.0f * scale;
    float stepSpacing = 2.0f * scale;
    float laneHeight = 30.0f * scale;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.frameBg);
    drawList->AddRectFilled(
        cursorPos,
        ImVec2(cursorPos.x + static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing), cursorPos.y + laneHeight),
        bgColor
    );

    // Probability indicators (diamonds)
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        if (!channel.steps[static_cast<size_t>(i)]) continue;

        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing) + stepSize / 2;
        float y = cursorPos.y + laneHeight / 2;
        float prob = channel.probabilities[static_cast<size_t>(i)];
        float size = 6.0f * scale * prob;

        // Color based on probability
        ImVec4 probColor = prob > 0.8f ? tokens.meterGreen :
                           prob > 0.4f ? tokens.meterYellow : tokens.meterRed;
        ImU32 color = ImGui::ColorConvertFloat4ToU32(probColor);

        // Diamond shape
        ImVec2 points[4] = {
            ImVec2(x, y - size),
            ImVec2(x + size, y),
            ImVec2(x, y + size),
            ImVec2(x - size, y)
        };
        drawList->AddConvexPolyFilled(points, 4, color);
    }

    // Interaction - click to toggle probability presets
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        if (!channel.steps[static_cast<size_t>(i)]) continue;

        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        ImGui::SetCursorScreenPos(ImVec2(x, cursorPos.y));
        ImGui::InvisibleButton("##prob", ImVec2(stepSize, laneHeight));

        if (ImGui::IsItemActive())
        {
            float mouseY = ImGui::GetIO().MousePos.y;
            float relY = 1.0f - (mouseY - cursorPos.y) / laneHeight;
            channel.probabilities[static_cast<size_t>(i)] = std::clamp(relY, 0.0f, 1.0f);
        }

        // Right-click for probability presets
        if (ImGui::IsItemClicked(1))
        {
            ImGui::OpenPopup("ProbPresets");
        }
    }

    // Probability presets popup
    if (ImGui::BeginPopup("ProbPresets"))
    {
        if (ImGui::MenuItem("100%")) { /* set focused step to 1.0 */ }
        if (ImGui::MenuItem("75%")) { /* set focused step to 0.75 */ }
        if (ImGui::MenuItem("50%")) { /* set focused step to 0.5 */ }
        if (ImGui::MenuItem("25%")) { /* set focused step to 0.25 */ }
        ImGui::EndPopup();
    }

    float totalWidth = static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing);
    ImGui::Dummy(ImVec2(totalWidth, laneHeight));
}

void ChannelRackPanel::drawConditionIndicators(int /*channelIndex*/, ChannelState& channel, const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    float stepSize = 20.0f * scale;
    float stepSpacing = 2.0f * scale;
    float laneHeight = 20.0f * scale;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.frameBg);
    drawList->AddRectFilled(
        cursorPos,
        ImVec2(cursorPos.x + static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing), cursorPos.y + laneHeight),
        bgColor
    );

    // Condition labels
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        if (!channel.steps[static_cast<size_t>(i)]) continue;

        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        StepCondition cond = channel.conditions[static_cast<size_t>(i)];
        int param = channel.conditionParams[static_cast<size_t>(i)];

        char label[8] = "";
        ImVec4 labelColor = tokens.text;

        switch (cond)
        {
            case StepCondition::Always:
                break;  // No label
            case StepCondition::FirstOnly:
                std::snprintf(label, sizeof(label), "1st");
                labelColor = ImVec4(0.3f, 0.7f, 0.9f, 1.0f);
                break;
            case StepCondition::Nth:
                std::snprintf(label, sizeof(label), "%dN", param);
                labelColor = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
                break;
            case StepCondition::EveryN:
                std::snprintf(label, sizeof(label), "/%d", param);
                labelColor = ImVec4(0.7f, 0.9f, 0.3f, 1.0f);
                break;
            case StepCondition::SkipM:
                std::snprintf(label, sizeof(label), "-%d", param);
                labelColor = ImVec4(0.9f, 0.5f, 0.5f, 1.0f);
                break;
            case StepCondition::Random:
                std::snprintf(label, sizeof(label), "?");
                labelColor = ImVec4(0.8f, 0.5f, 0.8f, 1.0f);
                break;
            case StepCondition::Fill:
                std::snprintf(label, sizeof(label), "F");
                labelColor = ImVec4(0.3f, 0.9f, 0.6f, 1.0f);
                break;
            case StepCondition::NotFill:
                std::snprintf(label, sizeof(label), "!F");
                labelColor = ImVec4(0.9f, 0.4f, 0.4f, 1.0f);
                break;
        }

        if (label[0] != '\0')
        {
            ImU32 color = ImGui::ColorConvertFloat4ToU32(labelColor);
            drawList->AddText(ImVec2(x + 2, cursorPos.y + 2), color, label);
        }
    }

    // Interaction - click to cycle conditions
    for (int i = 0; i < stepsPerPattern_; ++i)
    {
        if (!channel.steps[static_cast<size_t>(i)]) continue;

        float x = cursorPos.x + static_cast<float>(i) * (stepSize + stepSpacing);
        ImGui::SetCursorScreenPos(ImVec2(x, cursorPos.y));
        ImGui::InvisibleButton("##cond", ImVec2(stepSize, laneHeight));

        if (ImGui::IsItemClicked(0))
        {
            // Cycle through conditions
            int condVal = static_cast<int>(channel.conditions[static_cast<size_t>(i)]);
            condVal = (condVal + 1) % 6;
            channel.conditions[static_cast<size_t>(i)] = static_cast<StepCondition>(condVal);
        }

        // Right-click for condition parameter
        if (ImGui::IsItemClicked(1))
        {
            ImGui::OpenPopup("CondParam");
        }
    }

    float totalWidth = static_cast<float>(stepsPerPattern_) * (stepSize + stepSpacing);
    ImGui::Dummy(ImVec2(totalWidth, laneHeight));
}

void ChannelRackPanel::drawChannelParams(int /*channelIndex*/, ChannelState& channel, const Theme& theme)
{
    float scale = theme.getDpiScale();
    (void)scale; // May use later

    ImGui::Text("Channel Parameters");
    ImGui::Separator();

    // Transpose
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Transpose", &channel.transpose, -24, 24);

    // Sample start offset
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Sample Start", &channel.sampleStartOffset, 0.0f, 1.0f);

    // Reverse
    ImGui::Checkbox("Reverse", &channel.reverse);

    // Retrigger rate
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Retrigger", &channel.retriggerRate, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Retrigger rate (0 = off)");

    // Channel swing
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Channel Swing", &channel.channelSwing, -1.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Per-channel swing override");

    ImGui::Separator();

    // Volume and Pan
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Volume", &channel.volume, 0.0f, 1.0f);

    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Pan", &channel.pan, 0.0f, 1.0f);
}

void ChannelRackPanel::generateFlam(int channelIndex, int step, int flamCount, float flamSpacing)
{
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channels_.size())) return;
    if (step < 0 || step >= stepsPerPattern_) return;

    auto& channel = channels_[static_cast<size_t>(channelIndex)];

    // Generate flam sub-hits with decreasing velocity
    for (int i = 0; i < flamCount; ++i)
    {
        int targetStep = step + i;
        if (targetStep >= stepsPerPattern_) break;

        channel.steps[static_cast<size_t>(targetStep)] = true;
        channel.velocities[static_cast<size_t>(targetStep)] =
            1.0f - (static_cast<float>(i) / static_cast<float>(flamCount)) * 0.5f;
        channel.microTimingOffsets[static_cast<size_t>(targetStep)] =
            static_cast<int>(static_cast<float>(i) * flamSpacing * 100.0f);
    }
}

void ChannelRackPanel::generateRoll(int channelIndex, int startStep, int endStep, int divisions)
{
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channels_.size())) return;
    if (startStep < 0 || endStep >= stepsPerPattern_ || startStep >= endStep) return;

    auto& channel = channels_[static_cast<size_t>(channelIndex)];

    // Generate roll by filling steps with subdivisions
    // This would ideally create sub-tick events, but for step sequencer
    // we fill in the visible steps
    for (int step = startStep; step <= endStep; ++step)
    {
        channel.steps[static_cast<size_t>(step)] = true;
        // Alternate velocity for roll effect
        channel.velocities[static_cast<size_t>(step)] =
            (step % 2 == 0) ? 0.9f : 0.7f;
    }

    (void)divisions; // Would use for sub-tick resolution
}

} // namespace daw::ui::imgui

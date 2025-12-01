#include "MixerPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace daw::ui::imgui
{

MixerPanel::MixerPanel()
{
    master_.name = "Master";
    master_.volume = 0.8f;
    createDemoChannels();
}

void MixerPanel::createDemoChannels()
{
    addChannel("Drums");
    addChannel("Bass");
    addChannel("Keys");
    addChannel("Lead");
    addChannel("Pad");
    addChannel("FX");
    
    // Set some demo values
    channels_[0].inserts = {"Compressor", "EQ"};
    channels_[1].inserts = {"Bass Amp"};
    channels_[2].inserts = {"Reverb"};
    channels_[3].inserts = {"Delay", "Chorus"};
    
    channels_[0].sends = {"Reverb", "Delay"};
    channels_[1].sends = {"Reverb"};
}

void MixerPanel::addChannel(const std::string& name)
{
    MixerChannel channel;
    channel.name = name;
    channels_.push_back(std::move(channel));
}

void MixerPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;
    
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    // Update meter animations
    updateMeters();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingSm * scale));
    
    if (ImGui::Begin("Mixer", &open))
    {
        float stripWidth = 80.0f * scale;
        float contentHeight = ImGui::GetContentRegionAvail().y;
        
        // Horizontal scrolling area for channels
        if (ImGui::BeginChild("##MixerChannels", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            // Draw channel strips
            for (size_t i = 0; i < channels_.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                
                if (ImGui::BeginChild("##Strip", ImVec2(stripWidth, contentHeight - 8 * scale), true))
                {
                    drawChannelStrip(static_cast<int>(i), channels_[i], theme);
                }
                ImGui::EndChild();
                
                ImGui::SameLine();
                ImGui::PopID();
            }
            
            // Separator before master
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(4 * scale, 0));
            ImGui::SameLine();
            
            // Master channel (slightly wider)
            float masterWidth = 100.0f * scale;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(
                tokens.childBg.x * 1.2f,
                tokens.childBg.y * 1.2f,
                tokens.childBg.z * 1.2f,
                tokens.childBg.w
            ));
            
            if (ImGui::BeginChild("##MasterStrip", ImVec2(masterWidth, contentHeight - 8 * scale), true))
            {
                drawChannelStrip(-1, master_, theme, true);
            }
            ImGui::EndChild();
            
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
}

void MixerPanel::drawChannelStrip(int /*index*/, MixerChannel& channel, const Theme& theme, bool isMaster)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    // Channel name
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::SetNextItemWidth(size.x);
    
    char nameBuf[64];
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", channel.name.c_str());
    if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
    {
        channel.name = nameBuf;
    }
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // Insert effects section
    if (!isMaster)
    {
        ImGui::Text("Inserts");
        ImGui::PushStyleColor(ImGuiCol_Button, tokens.frameBg);
        for (size_t i = 0; i < 4; ++i)
        {
            std::string insertName = (i < channel.inserts.size()) ? channel.inserts[i] : "---";
            ImGui::SetNextItemWidth(size.x);
            if (ImGui::Button(insertName.c_str(), ImVec2(size.x, 0)))
            {
                // TODO: Open insert selector
            }
        }
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // Sends
        ImGui::Text("Sends");
        ImGui::PushStyleColor(ImGuiCol_Button, tokens.frameBg);
        for (size_t i = 0; i < 2; ++i)
        {
            std::string sendName = (i < channel.sends.size()) ? channel.sends[i] : "---";
            ImGui::SetNextItemWidth(size.x);
            if (ImGui::Button(sendName.c_str(), ImVec2(size.x, 0)))
            {
                // TODO: Open send selector
            }
        }
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
    }
    
    // Pan knob (simplified as slider)
    ImGui::Text("Pan");
    ImGui::SetNextItemWidth(size.x);
    float panDisplay = (channel.pan - 0.5f) * 200.0f;  // -100 to +100
    if (ImGui::SliderFloat("##Pan", &panDisplay, -100.0f, 100.0f, "%.0f"))
    {
        channel.pan = (panDisplay / 200.0f) + 0.5f;
    }
    
    // Meter and fader area
    ImGui::Spacing();
    
    float meterFaderHeight = std::max(100.0f * scale, size.y - ImGui::GetCursorPosY() - 60 * scale);
    float meterWidth = 20.0f * scale;
    float faderWidth = size.x - meterWidth - 8 * scale;
    
    ImGui::BeginGroup();
    
    // Meter (on left)
    drawMeter(channel, theme, meterWidth, meterFaderHeight);
    
    ImGui::SameLine();
    
    // Fader (on right)
    drawFader(channel, theme, faderWidth, meterFaderHeight);
    
    ImGui::EndGroup();
    
    // Volume readout
    float dbValue = (channel.volume > 0.0f) ? 20.0f * std::log10(channel.volume) : -60.0f;
    char dbBuf[16];
    std::snprintf(dbBuf, sizeof(dbBuf), "%.1f dB", dbValue);
    
    float textWidth = ImGui::CalcTextSize(dbBuf).x;
    ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
    ImGui::TextDisabled("%s", dbBuf);
    
    ImGui::Spacing();
    
    // Mute/Solo/Arm buttons
    float buttonWidth = (size.x - 8 * scale) / 3.0f;
    
    // Mute
    ImVec4 muteColor = channel.muted ? ImVec4(0.8f, 0.3f, 0.3f, 1.0f) : tokens.button;
    ImGui::PushStyleColor(ImGuiCol_Button, muteColor);
    if (ImGui::Button("M", ImVec2(buttonWidth, 0)))
    {
        channel.muted = !channel.muted;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mute");
    
    ImGui::SameLine();
    
    // Solo
    ImVec4 soloColor = channel.soloed ? ImVec4(0.9f, 0.8f, 0.2f, 1.0f) : tokens.button;
    ImGui::PushStyleColor(ImGuiCol_Button, soloColor);
    if (ImGui::Button("S", ImVec2(buttonWidth, 0)))
    {
        channel.soloed = !channel.soloed;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Solo");
    
    ImGui::SameLine();
    
    // Record arm (not for master)
    if (!isMaster)
    {
        ImVec4 armColor = channel.armed ? tokens.recordButton : tokens.button;
        ImGui::PushStyleColor(ImGuiCol_Button, armColor);
        if (ImGui::Button("R", ImVec2(buttonWidth, 0)))
        {
            channel.armed = !channel.armed;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Record Arm");
    }
}

void MixerPanel::drawMeter(const MixerChannel& channel, const Theme& theme, float width, float height)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    // Background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(tokens.meterBackground);
    drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bgColor, 2 * scale);
    
    float barWidth = (width - 4 * scale) / 2.0f;
    float margin = 2 * scale;
    
    // Draw meter segments for each channel
    auto drawMeterBar = [&](float x, float peak, float rms) {
        // RMS bar
        float rmsHeight = height * rms;
        ImVec4 rmsColor = (rms > 0.9f) ? tokens.meterRed :
                          (rms > 0.7f) ? tokens.meterYellow : tokens.meterGreen;
        rmsColor.w *= 0.7f;  // Slightly transparent
        ImU32 rmsCol = ImGui::ColorConvertFloat4ToU32(rmsColor);
        
        drawList->AddRectFilled(
            ImVec2(x, pos.y + height - rmsHeight),
            ImVec2(x + barWidth, pos.y + height),
            rmsCol
        );
        
        // Peak indicator
        float peakY = pos.y + height - (height * peak);
        ImVec4 peakColor = (peak > 0.9f) ? tokens.meterRed :
                           (peak > 0.7f) ? tokens.meterYellow : tokens.meterGreen;
        ImU32 peakCol = ImGui::ColorConvertFloat4ToU32(peakColor);
        
        drawList->AddRectFilled(
            ImVec2(x, peakY),
            ImVec2(x + barWidth, peakY + 2 * scale),
            peakCol
        );
    };
    
    // Left channel
    drawMeterBar(pos.x + margin, channel.peakL, channel.rmsL);
    
    // Right channel
    drawMeterBar(pos.x + margin + barWidth, channel.peakR, channel.rmsR);
    
    // Clipping indicator at top
    if (channel.peakL >= 1.0f || channel.peakR >= 1.0f)
    {
        ImU32 clipColor = ImGui::ColorConvertFloat4ToU32(tokens.meterRed);
        drawList->AddRectFilled(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + width, pos.y + 4 * scale),
            clipColor
        );
    }
    
    // Advance cursor
    ImGui::Dummy(ImVec2(width, height));
}

void MixerPanel::drawFader(MixerChannel& channel, const Theme& theme, float width, float height)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    // Fader track
    float trackX = pos.x + width * 0.5f - 2 * scale;
    ImU32 trackColor = ImGui::ColorConvertFloat4ToU32(tokens.frameBg);
    drawList->AddRectFilled(
        ImVec2(trackX, pos.y),
        ImVec2(trackX + 4 * scale, pos.y + height),
        trackColor,
        2 * scale
    );
    
    // Scale markings
    ImU32 markColor = ImGui::ColorConvertFloat4ToU32(tokens.textDisabled);
    float dbMarks[] = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
    for (float db : dbMarks)
    {
        float linear = std::pow(10.0f, db / 20.0f);
        float y = pos.y + height * (1.0f - linear);
        drawList->AddLine(
            ImVec2(pos.x, y),
            ImVec2(pos.x + 8 * scale, y),
            markColor
        );
    }
    
    // Fader handle
    float handleHeight = 20.0f * scale;
    float handleY = pos.y + height * (1.0f - channel.volume) - handleHeight * 0.5f;
    handleY = std::clamp(handleY, pos.y, pos.y + height - handleHeight);
    
    ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(tokens.sliderGrab);
    ImU32 handleBorder = ImGui::ColorConvertFloat4ToU32(tokens.border);
    
    drawList->AddRectFilled(
        ImVec2(pos.x, handleY),
        ImVec2(pos.x + width, handleY + handleHeight),
        handleColor,
        4 * scale
    );
    drawList->AddRect(
        ImVec2(pos.x, handleY),
        ImVec2(pos.x + width, handleY + handleHeight),
        handleBorder,
        4 * scale
    );
    
    // Center line on handle
    float centerY = handleY + handleHeight * 0.5f;
    drawList->AddLine(
        ImVec2(pos.x + 4 * scale, centerY),
        ImVec2(pos.x + width - 4 * scale, centerY),
        handleBorder
    );
    
    // Interaction
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##fader", ImVec2(width, height));
    
    if (ImGui::IsItemActive())
    {
        float mouseY = ImGui::GetIO().MousePos.y;
        float relY = 1.0f - (mouseY - pos.y) / height;
        channel.volume = std::clamp(relY, 0.0f, 1.0f);
        
        if (onVolumeChanged_)
        {
            // Find channel index (would need to track this better)
            onVolumeChanged_(-1, channel.volume);
        }
    }
    
    // Double-click to reset
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        channel.volume = 0.8f;  // Reset to 0dB
    }
}

void MixerPanel::updateMeters()
{
    // Simulate meter animation (in real implementation, get from audio engine)
    float dt = ImGui::GetIO().DeltaTime;
    float peakFalloff = 3.0f;  // dB per second equivalent
    
    auto updateChannel = [&](MixerChannel& ch) {
        // Simulate some activity
        float activity = 0.3f + 0.2f * static_cast<float>(std::sin(ImGui::GetTime() * 2.0 + 
            static_cast<double>(std::hash<std::string>{}(ch.name) % 100)));
        
        if (!ch.muted)
        {
            // Occasional peaks
            float peakChance = 0.05f;
            if (static_cast<float>(rand()) / RAND_MAX < peakChance)
            {
                ch.peakL = std::min(1.0f, activity + 0.3f * static_cast<float>(rand()) / RAND_MAX);
                ch.peakR = std::min(1.0f, activity + 0.3f * static_cast<float>(rand()) / RAND_MAX);
            }
            
            // RMS follows activity more closely
            ch.rmsL = activity * ch.volume;
            ch.rmsR = activity * ch.volume;
        }
        else
        {
            ch.rmsL = 0.0f;
            ch.rmsR = 0.0f;
        }
        
        // Peak falloff
        ch.peakL = std::max(ch.rmsL, ch.peakL - peakFalloff * dt);
        ch.peakR = std::max(ch.rmsR, ch.peakR - peakFalloff * dt);
    };
    
    for (auto& ch : channels_)
    {
        updateChannel(ch);
    }
    
    // Master is sum of all channels (simplified)
    master_.rmsL = 0.0f;
    master_.rmsR = 0.0f;
    for (const auto& ch : channels_)
    {
        if (!ch.muted)
        {
            master_.rmsL = std::min(1.0f, master_.rmsL + ch.rmsL * 0.3f);
            master_.rmsR = std::min(1.0f, master_.rmsR + ch.rmsR * 0.3f);
        }
    }
    master_.rmsL *= master_.volume;
    master_.rmsR *= master_.volume;
    master_.peakL = std::max(master_.rmsL, master_.peakL - peakFalloff * dt);
    master_.peakR = std::max(master_.rmsR, master_.peakR - peakFalloff * dt);
}

} // namespace daw::ui::imgui

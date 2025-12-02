#include "PluginWindow.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>

namespace daw::ui::imgui
{

// =============================================================================
// PluginWindow Implementation
// =============================================================================

PluginWindow::PluginWindow()
{
    // Create demo state
    state_ = std::make_unique<PluginState>();
    state_->name = "Demo Synth";
    state_->vendor = "CPPMusic";
    state_->version = "1.0.0";
    state_->format = "Internal";

    // Add demo parameters
    state_->parameters.push_back({0, "Cutoff", 0.5f, 0.0f, 1.0f, 0.5f, "Hz", false, false, -1});
    state_->parameters.push_back({1, "Resonance", 0.3f, 0.0f, 1.0f, 0.3f, "", false, false, -1});
    state_->parameters.push_back({2, "Attack", 0.1f, 0.0f, 1.0f, 0.1f, "ms", false, false, -1});
    state_->parameters.push_back({3, "Decay", 0.3f, 0.0f, 1.0f, 0.3f, "ms", false, false, -1});
    state_->parameters.push_back({4, "Sustain", 0.7f, 0.0f, 1.0f, 0.7f, "", false, false, -1});
    state_->parameters.push_back({5, "Release", 0.4f, 0.0f, 1.0f, 0.4f, "ms", false, false, -1});
    state_->parameters.push_back({6, "Volume", 0.8f, 0.0f, 1.0f, 0.8f, "dB", false, false, -1});

    // Add demo presets
    state_->presets.push_back({"Init", "CPPMusic", "Init", {}, {0.5f, 0.3f, 0.1f, 0.3f, 0.7f, 0.4f, 0.8f}, true, false});
    state_->presets.push_back({"Soft Pad", "CPPMusic", "Pads", {"warm", "soft"}, {0.3f, 0.2f, 0.5f, 0.6f, 0.8f, 0.7f, 0.7f}, true, true});
    state_->presets.push_back({"Sharp Lead", "CPPMusic", "Leads", {"bright", "cutting"}, {0.8f, 0.6f, 0.0f, 0.1f, 0.5f, 0.2f, 0.9f}, true, false});
    state_->presets.push_back({"Deep Bass", "CPPMusic", "Bass", {"sub", "deep"}, {0.2f, 0.4f, 0.0f, 0.2f, 1.0f, 0.3f, 0.8f}, true, true});
}

void PluginWindow::draw(bool& open, const Theme& theme)
{
    if (!open || !state_) return;

    [[maybe_unused]] [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::SetNextWindowSize(ImVec2(windowWidth_ * scale, windowHeight_ * scale), ImGuiCond_FirstUseEver);

    std::string windowTitle = state_->name + "###PluginWindow" + std::to_string(state_->instanceId);

    if (ImGui::Begin(windowTitle.c_str(), &open, ImGuiWindowFlags_MenuBar))
    {
        // Menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save Preset...")) {}
                if (ImGui::MenuItem("Load Preset...")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Copy State")) {}
                if (ImGui::MenuItem("Paste State")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !undoStack_.empty())) undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !redoStack_.empty())) redo();
                ImGui::Separator();
                if (ImGui::MenuItem("Reset to Default")) {
                    for (auto& param : state_->parameters) {
                        param.value = param.defaultValue;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Compact View", nullptr, &compactView_);
                ImGui::MenuItem("Show Preset Browser", nullptr, &state_->showPresetBrowser);
                ImGui::MenuItem("Show Parameter List", nullptr, &state_->showParameterList);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        drawTitleBar(theme);
        drawPresetSelector(theme);

        if (state_->showPresetBrowser)
        {
            drawPresetBrowser(theme);
        }

        ImGui::Separator();
        drawBypassMix(theme);
        ImGui::Separator();
        drawABComparison(theme);
        ImGui::Separator();

        // Plugin content area (placeholder for actual plugin UI)
        drawPluginContent(theme);

        if (state_->showParameterList)
        {
            ImGui::Separator();
            drawParameterList(theme);
        }
    }
    ImGui::End();
}

void PluginWindow::drawTitleBar(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();
    [[maybe_unused]] float scale = theme.getDpiScale();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::Text("%s by %s (%s)", state_->name.c_str(), state_->vendor.c_str(), state_->format.c_str());
    ImGui::PopStyleColor();
}

void PluginWindow::drawPresetSelector(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::PushItemWidth(200 * scale);

    std::string currentPreset = state_->currentPresetIndex >= 0 &&
                                state_->currentPresetIndex < static_cast<int>(state_->presets.size())
        ? state_->presets[static_cast<size_t>(state_->currentPresetIndex)].name
        : "No Preset";

    if (ImGui::BeginCombo("##PresetSelector", currentPreset.c_str()))
    {
        for (size_t i = 0; i < state_->presets.size(); ++i)
        {
            bool isSelected = (static_cast<int>(i) == state_->currentPresetIndex);
            std::string label = state_->presets[i].name;
            if (state_->presets[i].isFavorite) label = "* " + label;

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                state_->currentPresetIndex = static_cast<int>(i);
                // Load preset values
                if (state_->presets[i].parameterValues.size() == state_->parameters.size())
                {
                    for (size_t j = 0; j < state_->parameters.size(); ++j)
                    {
                        state_->parameters[j].value = state_->presets[i].parameterValues[j];
                    }
                }
                if (onPresetChanged_) onPresetChanged_(static_cast<int>(i));
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("<##PrevPreset"))
    {
        if (state_->currentPresetIndex > 0)
        {
            state_->currentPresetIndex--;
            if (onPresetChanged_) onPresetChanged_(state_->currentPresetIndex);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(">##NextPreset"))
    {
        if (state_->currentPresetIndex < static_cast<int>(state_->presets.size()) - 1)
        {
            state_->currentPresetIndex++;
            if (onPresetChanged_) onPresetChanged_(state_->currentPresetIndex);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Browse"))
    {
        state_->showPresetBrowser = !state_->showPresetBrowser;
    }
}

void PluginWindow::drawPresetBrowser(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::BeginChild("##PresetBrowser", ImVec2(0, 150 * scale), true);

    // Search bar
    ImGui::InputTextWithHint("##PresetSearch", "Search presets...", presetSearchBuffer_, sizeof(presetSearchBuffer_));

    // Category filter
    ImGui::SameLine();
    if (ImGui::BeginCombo("##CategoryFilter", presetCategoryFilter_.empty() ? "All Categories" : presetCategoryFilter_.c_str()))
    {
        if (ImGui::Selectable("All Categories", presetCategoryFilter_.empty()))
        {
            presetCategoryFilter_.clear();
        }

        std::vector<std::string> categories;
        for (const auto& preset : state_->presets)
        {
            if (std::find(categories.begin(), categories.end(), preset.category) == categories.end())
            {
                categories.push_back(preset.category);
            }
        }

        for (const auto& cat : categories)
        {
            if (ImGui::Selectable(cat.c_str(), presetCategoryFilter_ == cat))
            {
                presetCategoryFilter_ = cat;
            }
        }
        ImGui::EndCombo();
    }

    // Preset list
    ImGui::BeginChild("##PresetList", ImVec2(0, 0), false);
    for (size_t i = 0; i < state_->presets.size(); ++i)
    {
        const auto& preset = state_->presets[i];

        // Filter by search
        if (presetSearchBuffer_[0] != '\0')
        {
            std::string searchLower = presetSearchBuffer_;
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
            std::string nameLower = preset.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(searchLower) == std::string::npos) continue;
        }

        // Filter by category
        if (!presetCategoryFilter_.empty() && preset.category != presetCategoryFilter_) continue;

        bool isSelected = (static_cast<int>(i) == state_->currentPresetIndex);
        std::string label = preset.name;
        if (preset.isFavorite) label = "* " + label;

        if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            state_->currentPresetIndex = static_cast<int>(i);
            if (ImGui::IsMouseDoubleClicked(0))
            {
                if (onPresetChanged_) onPresetChanged_(static_cast<int>(i));
            }
        }
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void PluginWindow::drawPluginContent(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    // Placeholder for actual plugin UI
    ImGui::BeginChild("##PluginContent", ImVec2(0, 200 * scale), true);

    ImGui::Text("Plugin UI Placeholder");
    ImGui::Separator();

    // Draw basic parameter knobs
    int paramsPerRow = compactView_ ? 6 : 4;
    for (size_t i = 0; i < state_->parameters.size(); ++i)
    {
        auto& param = state_->parameters[i];

        if (i % paramsPerRow != 0) ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::PushID(static_cast<int>(i));

        // Simple slider as knob placeholder
        ImGui::VSliderFloat("##v", ImVec2(30 * scale, 80 * scale), &param.value, param.minValue, param.maxValue, "");

        if (ImGui::IsItemActive() || ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s: %.2f%s", param.name.c_str(), param.value, param.unit.c_str());
        }

        // Right-click context menu
        if (ImGui::BeginPopupContextItem("param_context"))
        {
            if (ImGui::MenuItem("Link to Controller"))
            {
                state_->linkingMode = true;
                state_->selectedParameterForLink = static_cast<int>(i);
                if (onLinkParameter_) onLinkParameter_(param.id);
            }
            if (ImGui::MenuItem("MIDI Learn"))
            {
                if (onMidiLearn_) onMidiLearn_(param.id);
            }
            if (ImGui::MenuItem("Create Automation Clip"))
            {
                param.isAutomated = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset to Default"))
            {
                pushUndo(param.id, param.value);
                param.value = param.defaultValue;
            }
            ImGui::EndPopup();
        }

        ImGui::TextUnformatted(param.name.c_str());

        ImGui::PopID();
        ImGui::EndGroup();
    }

    ImGui::EndChild();
}

void PluginWindow::drawParameterList(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::BeginChild("##ParameterList", ImVec2(0, 150 * scale), true);

    ImGui::Text("Parameters");
    ImGui::Separator();

    for (auto& param : state_->parameters)
    {
        ImGui::PushID(param.id);

        // Parameter name
        ImGui::Text("%s", param.name.c_str());
        ImGui::SameLine(150 * scale);

        // Value slider
        ImGui::PushItemWidth(100 * scale);
        if (ImGui::SliderFloat("##value", &param.value, param.minValue, param.maxValue))
        {
            if (onParameterChanged_) onParameterChanged_(param.id, param.value);
        }
        ImGui::PopItemWidth();

        // Automation indicator
        ImGui::SameLine();
        if (param.isAutomated)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[A]");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[ ]");
        }

        // Link indicator
        ImGui::SameLine();
        if (param.isLinked)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[L]");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[ ]");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void PluginWindow::drawBypassMix(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    // Bypass button
    if (state_->bypass)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    }
    if (ImGui::Button(state_->bypass ? "BYPASSED" : "Bypass"))
    {
        state_->bypass = !state_->bypass;
    }
    if (state_->bypass)
    {
        ImGui::PopStyleColor();
    }

    // Dry/wet mix
    ImGui::SameLine();
    ImGui::Text("Mix:");
    ImGui::SameLine();
    ImGui::PushItemWidth(100 * scale);
    ImGui::SliderFloat("##mix", &state_->mix, 0.0f, 1.0f, "%.0f%%");
    ImGui::PopItemWidth();
}

void PluginWindow::drawABComparison(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();

    if (ImGui::Button(isStateA_ ? "[A] / B" : "A / [B]"))
    {
        if (isStateA_)
        {
            saveStateToA();
            loadStateFromB();
        }
        else
        {
            saveStateToB();
            loadStateFromA();
        }
        isStateA_ = !isStateA_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy A->B"))
    {
        saveStateToA();
        stateB_ = stateA_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy B->A"))
    {
        saveStateToB();
        stateA_ = stateB_;
    }
}

void PluginWindow::saveStateToA()
{
    stateA_.clear();
    for (const auto& param : state_->parameters)
    {
        stateA_.push_back(param.value);
    }
}

void PluginWindow::saveStateToB()
{
    stateB_.clear();
    for (const auto& param : state_->parameters)
    {
        stateB_.push_back(param.value);
    }
}

void PluginWindow::loadStateFromA()
{
    if (stateA_.size() == state_->parameters.size())
    {
        for (size_t i = 0; i < state_->parameters.size(); ++i)
        {
            state_->parameters[i].value = stateA_[i];
        }
    }
}

void PluginWindow::loadStateFromB()
{
    if (stateB_.size() == state_->parameters.size())
    {
        for (size_t i = 0; i < state_->parameters.size(); ++i)
        {
            state_->parameters[i].value = stateB_[i];
        }
    }
}

void PluginWindow::pushUndo(int paramId, float oldValue)
{
    undoStack_.push_back({paramId, oldValue});
    redoStack_.clear();
}

void PluginWindow::undo()
{
    if (undoStack_.empty()) return;

    auto [paramId, oldValue] = undoStack_.back();
    undoStack_.pop_back();

    for (auto& param : state_->parameters)
    {
        if (param.id == paramId)
        {
            redoStack_.push_back({paramId, param.value});
            param.value = oldValue;
            break;
        }
    }
}

void PluginWindow::redo()
{
    if (redoStack_.empty()) return;

    auto [paramId, newValue] = redoStack_.back();
    redoStack_.pop_back();

    for (auto& param : state_->parameters)
    {
        if (param.id == paramId)
        {
            undoStack_.push_back({paramId, param.value});
            param.value = newValue;
            break;
        }
    }
}

// =============================================================================
// PluginPicker Implementation
// =============================================================================

PluginPicker::PluginPicker()
{
    // Add demo plugins
    plugins_.push_back({"demo_synth", "Demo Synth", "CPPMusic", "Synthesizers", "Internal", true, true, 10});
    plugins_.push_back({"demo_sampler", "Demo Sampler", "CPPMusic", "Samplers", "Internal", true, false, 5});
    plugins_.push_back({"demo_eq", "Demo EQ", "CPPMusic", "EQ", "Internal", false, false, 8});
    plugins_.push_back({"demo_comp", "Demo Compressor", "CPPMusic", "Dynamics", "Internal", false, true, 12});
    plugins_.push_back({"demo_reverb", "Demo Reverb", "CPPMusic", "Reverb", "Internal", false, false, 7});
    plugins_.push_back({"demo_delay", "Demo Delay", "CPPMusic", "Delay", "Internal", false, false, 6});

    // Build category list
    for (const auto& plugin : plugins_)
    {
        if (std::find(categories_.begin(), categories_.end(), plugin.category) == categories_.end())
        {
            categories_.push_back(plugin.category);
        }
    }

    filterPlugins();
}

void PluginPicker::draw(bool& open, const Theme& theme)
{
    if (!open) return;

    [[maybe_unused]] const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();

    ImGui::SetNextWindowSize(ImVec2(500 * scale, 400 * scale), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Plugin Picker", &open))
    {
        drawTabs(theme);
        drawSearchBar(theme);

        // Two-column layout
        ImGui::BeginChild("##Left", ImVec2(150 * scale, 0), true);
        drawCategoryList(theme);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##Right", ImVec2(0, 0), true);
        drawPluginList(theme);
        ImGui::EndChild();
    }
    ImGui::End();
}

void PluginPicker::drawTabs([[maybe_unused]] const Theme& theme)
{
    if (ImGui::BeginTabBar("##PluginTabs"))
    {
        if (ImGui::BeginTabItem("Generators"))
        {
            showGenerators_ = true;
            showEffects_ = false;
            filterPlugins();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Effects"))
        {
            showGenerators_ = false;
            showEffects_ = true;
            filterPlugins();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("All"))
        {
            showGenerators_ = true;
            showEffects_ = true;
            filterPlugins();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Favorites"))
        {
            showFavoritesOnly_ = true;
            filterPlugins();
            ImGui::EndTabItem();
        }
        else
        {
            showFavoritesOnly_ = false;
        }
        ImGui::EndTabBar();
    }
}

void PluginPicker::drawSearchBar([[maybe_unused]] const Theme& theme)
{
    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search plugins...", searchBuffer_, sizeof(searchBuffer_)))
    {
        filterPlugins();
    }
    ImGui::PopItemWidth();
}

void PluginPicker::drawCategoryList([[maybe_unused]] const Theme& theme)
{
    if (ImGui::Selectable("All", selectedCategory_.empty()))
    {
        selectedCategory_.clear();
        filterPlugins();
    }

    ImGui::Separator();

    for (const auto& category : categories_)
    {
        if (ImGui::Selectable(category.c_str(), selectedCategory_ == category))
        {
            selectedCategory_ = category;
            filterPlugins();
        }
    }
}

void PluginPicker::drawPluginList(const Theme& theme)
{
    [[maybe_unused]] const auto& tokens = theme.getTokens();

    for (const auto& plugin : filteredPlugins_)
    {
        ImGui::PushID(plugin.id.c_str());

        // Favorite star
        if (plugin.isFavorite)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "*");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " ");
        }
        ImGui::SameLine();

        // Plugin name
        if (ImGui::Selectable(plugin.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
            {
                if (onPluginSelected_) onPluginSelected_(plugin.id);
            }
        }

        // Vendor on same line
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", plugin.vendor.c_str());

        ImGui::PopID();
    }
}

void PluginPicker::filterPlugins()
{
    filteredPlugins_.clear();

    std::string searchLower = searchBuffer_;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    for (const auto& plugin : plugins_)
    {
        // Filter by type
        if (plugin.isGenerator && !showGenerators_) continue;
        if (!plugin.isGenerator && !showEffects_) continue;

        // Filter by favorites
        if (showFavoritesOnly_ && !plugin.isFavorite) continue;

        // Filter by category
        if (!selectedCategory_.empty() && plugin.category != selectedCategory_) continue;

        // Filter by search
        if (!searchLower.empty())
        {
            std::string nameLower = plugin.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(searchLower) == std::string::npos) continue;
        }

        filteredPlugins_.push_back(plugin);
    }

    // Sort by use count (most used first)
    std::sort(filteredPlugins_.begin(), filteredPlugins_.end(),
        [](const PluginInfo& a, const PluginInfo& b) {
            return a.useCount > b.useCount;
        });
}

void PluginPicker::setPlugins(const std::vector<std::pair<std::string, std::string>>& plugins)
{
    plugins_.clear();
    for (const auto& [id, name] : plugins)
    {
        plugins_.push_back({id, name, "", "", "", false, false, 0});
    }
    filterPlugins();
}

} // namespace daw::ui::imgui

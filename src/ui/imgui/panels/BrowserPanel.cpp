#include "BrowserPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cctype>

namespace daw::ui::imgui
{

BrowserPanel::BrowserPanel()
{
    std::memset(searchBuffer_, 0, sizeof(searchBuffer_));
    
    // Initialize filter chips
    filterChips_ = {
        {"Audio", false},
        {"MIDI", false},
        {"Presets", false},
        {"Plugins", false}
    };
    
    // Create demo content for preview
    createDemoContent();
}

void BrowserPanel::createDemoContent()
{
    // Samples folder
    auto samples = std::make_unique<BrowserItem>();
    samples->name = "Samples";
    samples->type = BrowserItemType::Folder;
    samples->path = "/samples";
    samples->isExpanded = true;
    
    // Add some sample items
    auto drums = std::make_unique<BrowserItem>();
    drums->name = "Drums";
    drums->type = BrowserItemType::Folder;
    drums->path = "/samples/drums";
    
    auto kick = std::make_unique<BrowserItem>();
    kick->name = "Kick_01.wav";
    kick->type = BrowserItemType::AudioFile;
    kick->path = "/samples/drums/Kick_01.wav";
    drums->children.push_back(std::move(kick));
    
    auto snare = std::make_unique<BrowserItem>();
    snare->name = "Snare_01.wav";
    snare->type = BrowserItemType::AudioFile;
    snare->path = "/samples/drums/Snare_01.wav";
    drums->children.push_back(std::move(snare));
    
    auto hihat = std::make_unique<BrowserItem>();
    hihat->name = "HiHat_Closed.wav";
    hihat->type = BrowserItemType::AudioFile;
    hihat->path = "/samples/drums/HiHat_Closed.wav";
    drums->children.push_back(std::move(hihat));
    
    samples->children.push_back(std::move(drums));
    
    // Synths folder
    auto synths = std::make_unique<BrowserItem>();
    synths->name = "Synths";
    synths->type = BrowserItemType::Folder;
    synths->path = "/samples/synths";
    
    auto pad = std::make_unique<BrowserItem>();
    pad->name = "Ambient_Pad.wav";
    pad->type = BrowserItemType::AudioFile;
    pad->path = "/samples/synths/Ambient_Pad.wav";
    synths->children.push_back(std::move(pad));
    
    samples->children.push_back(std::move(synths));
    
    rootItems_.push_back(std::move(samples));
    
    // Presets folder
    auto presets = std::make_unique<BrowserItem>();
    presets->name = "Presets";
    presets->type = BrowserItemType::Folder;
    presets->path = "/presets";
    
    auto synthPresets = std::make_unique<BrowserItem>();
    synthPresets->name = "Synthesizer";
    synthPresets->type = BrowserItemType::Folder;
    synthPresets->path = "/presets/synthesizer";
    
    auto preset1 = std::make_unique<BrowserItem>();
    preset1->name = "Warm Lead.preset";
    preset1->type = BrowserItemType::Preset;
    preset1->path = "/presets/synthesizer/Warm Lead.preset";
    synthPresets->children.push_back(std::move(preset1));
    
    auto preset2 = std::make_unique<BrowserItem>();
    preset2->name = "Bass Growl.preset";
    preset2->type = BrowserItemType::Preset;
    preset2->path = "/presets/synthesizer/Bass Growl.preset";
    synthPresets->children.push_back(std::move(preset2));
    
    presets->children.push_back(std::move(synthPresets));
    rootItems_.push_back(std::move(presets));
    
    // Plugins folder
    auto plugins = std::make_unique<BrowserItem>();
    plugins->name = "Plugins";
    plugins->type = BrowserItemType::Folder;
    plugins->path = "/plugins";
    
    auto effectsFolder = std::make_unique<BrowserItem>();
    effectsFolder->name = "Effects";
    effectsFolder->type = BrowserItemType::Folder;
    effectsFolder->path = "/plugins/effects";
    
    auto reverb = std::make_unique<BrowserItem>();
    reverb->name = "Aurora Reverb";
    reverb->type = BrowserItemType::Plugin;
    reverb->path = "/plugins/effects/AuroraReverb.vst3";
    effectsFolder->children.push_back(std::move(reverb));
    
    auto eq = std::make_unique<BrowserItem>();
    eq->name = "Analog EQ";
    eq->type = BrowserItemType::Plugin;
    eq->path = "/plugins/effects/AnalogEQ.vst3";
    effectsFolder->children.push_back(std::move(eq));
    
    plugins->children.push_back(std::move(effectsFolder));
    rootItems_.push_back(std::move(plugins));
}

void BrowserPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;
    
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingSm * scale));
    
    if (ImGui::Begin("Browser", &open))
    {
        drawSearchBar(theme);
        drawFilterChips(theme);
        
        ImGui::Separator();
        
        // Tree view
        if (ImGui::BeginChild("##BrowserTree", ImVec2(0, 0), false))
        {
            for (const auto& item : rootItems_)
            {
                if (matchesFilter(*item))
                {
                    drawTreeItem(*item, theme);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
}

void BrowserPanel::drawSearchBar(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale));
    
    bool changed = ImGui::InputTextWithHint("##BrowserSearch", "Search...", 
                                             searchBuffer_, sizeof(searchBuffer_));
    
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
    
    // Debounced search
    if (changed)
    {
        searchDebounceTime_ = 0.3f;  // 300ms debounce
    }
    
    if (searchDebounceTime_ > 0.0f)
    {
        searchDebounceTime_ -= ImGui::GetIO().DeltaTime;
        if (searchDebounceTime_ <= 0.0f)
        {
            lastSearch_ = searchBuffer_;
        }
    }
}

void BrowserPanel::drawFilterChips(const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::Spacing();
    
    for (size_t i = 0; i < filterChips_.size(); ++i)
    {
        auto& chip = filterChips_[i];
        
        if (i > 0) ImGui::SameLine();
        
        // Style for active/inactive chips
        if (chip.active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, tokens.buttonActive);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens.buttonHovered);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, tokens.button);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens.buttonHovered);
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, tokens.radiusLg * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tokens.spacingSm * scale, tokens.spacingXs * scale / 2));
        
        if (ImGui::SmallButton(chip.label.c_str()))
        {
            chip.active = !chip.active;
        }
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }
    
    ImGui::Spacing();
}

void BrowserPanel::drawTreeItem(const BrowserItem& item, const Theme& theme)
{
    const auto& tokens = theme.getTokens();
    
    const char* icon = getIconForType(item.type);
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (item.children.empty() && item.type != BrowserItemType::Folder)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    if (&item == selectedItem_)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // Loading indicator
    if (item.isLoading)
    {
        ImGui::TextDisabled("%s %s (loading...)", icon, item.name.c_str());
        return;
    }
    
    // Color code by type
    ImVec4 itemColor = tokens.text;
    switch (item.type)
    {
        case BrowserItemType::AudioFile:
            itemColor = ImVec4(0.4f, 0.8f, 0.9f, 1.0f);
            break;
        case BrowserItemType::MidiFile:
            itemColor = ImVec4(0.9f, 0.7f, 0.4f, 1.0f);
            break;
        case BrowserItemType::Preset:
            itemColor = ImVec4(0.7f, 0.9f, 0.5f, 1.0f);
            break;
        case BrowserItemType::Plugin:
            itemColor = ImVec4(0.9f, 0.5f, 0.7f, 1.0f);
            break;
        default:
            break;
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, itemColor);
    
    std::string label = std::string(icon) + " " + item.name;
    bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);
    
    ImGui::PopStyleColor();
    
    // Handle selection
    if (ImGui::IsItemClicked())
    {
        selectedItem_ = &item;
        if (onItemSelected_) onItemSelected_(item);
    }
    
    // Handle double-click
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (onItemActivated_) onItemActivated_(item);
    }
    
    // Drag source for drag-and-drop
    if (item.type != BrowserItemType::Folder && ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("BROWSER_ITEM", &item, sizeof(void*));
        ImGui::Text("%s %s", icon, item.name.c_str());
        ImGui::EndDragDropSource();
    }
    
    // Context menu
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Open")) { if (onItemActivated_) onItemActivated_(item); }
        if (item.type == BrowserItemType::AudioFile)
        {
            if (ImGui::MenuItem("Preview")) { /* TODO */ }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show in Explorer")) { /* TODO */ }
        ImGui::EndPopup();
    }
    
    // Draw children
    if (nodeOpen && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        for (const auto& child : item.children)
        {
            if (matchesFilter(*child))
            {
                drawTreeItem(*child, theme);
            }
        }
        ImGui::TreePop();
    }
}

const char* BrowserPanel::getIconForType(BrowserItemType type)
{
    switch (type)
    {
        case BrowserItemType::Folder:    return "[D]";
        case BrowserItemType::AudioFile: return "[W]";
        case BrowserItemType::MidiFile:  return "[M]";
        case BrowserItemType::Preset:    return "[P]";
        case BrowserItemType::Plugin:    return "[X]";
        case BrowserItemType::Project:   return "[*]";
        default:                         return "[?]";
    }
}

bool BrowserPanel::matchesFilter(const BrowserItem& item) const
{
    // Check search filter
    if (lastSearch_.empty() == false)
    {
        std::string lowerName = item.name;
        std::string lowerSearch = lastSearch_;
        for (auto& c : lowerName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (auto& c : lowerSearch) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        
        if (lowerName.find(lowerSearch) == std::string::npos)
        {
            // Check if any children match
            for (const auto& child : item.children)
            {
                if (matchesFilter(*child)) return true;
            }
            return false;
        }
    }
    
    // Check type filters
    bool anyFilterActive = false;
    for (const auto& chip : filterChips_)
    {
        if (chip.active)
        {
            anyFilterActive = true;
            break;
        }
    }
    
    if (!anyFilterActive) return true;
    
    // Folders always show if they have matching children
    if (item.type == BrowserItemType::Folder)
    {
        for (const auto& child : item.children)
        {
            if (matchesFilter(*child)) return true;
        }
        return false;
    }
    
    // Check specific type filters
    for (const auto& chip : filterChips_)
    {
        if (!chip.active) continue;
        
        if (chip.label == "Audio" && item.type == BrowserItemType::AudioFile) return true;
        if (chip.label == "MIDI" && item.type == BrowserItemType::MidiFile) return true;
        if (chip.label == "Presets" && item.type == BrowserItemType::Preset) return true;
        if (chip.label == "Plugins" && item.type == BrowserItemType::Plugin) return true;
    }
    
    return false;
}

void BrowserPanel::addRootItem(std::unique_ptr<BrowserItem> item)
{
    rootItems_.push_back(std::move(item));
}

void BrowserPanel::clear()
{
    rootItems_.clear();
    selectedItem_ = nullptr;
}

} // namespace daw::ui::imgui

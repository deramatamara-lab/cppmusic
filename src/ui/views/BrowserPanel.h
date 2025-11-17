#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/ProjectModel.h"
#include "../../audio/engine/EngineContext.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Browser/library panel
 * 
 * Displays tracks, instruments, plugins, and samples.
 * Supports selection and double-click actions.
 * Follows DAW_DEV_RULES: uses design system, responsive layout.
 */
class BrowserPanel : public juce::Component
{
public:
    BrowserPanel(std::shared_ptr<daw::project::ProjectModel> projectModel,
                 std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~BrowserPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    
    juce::TreeView treeView;
    juce::TextButton addTrackButton;
    
    void setupUI();
    void addTrackButtonClicked();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

} // namespace daw::ui::views


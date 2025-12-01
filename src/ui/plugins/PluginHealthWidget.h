/**
 * @file PluginHealthWidget.h
 * @brief Plugin health and sandbox status widget header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::plugins {

/**
 * @brief Plugin health monitoring widget
 *
 * Features:
 * - CPU usage per plugin
 * - Latency monitoring
 * - Crash detection
 * - Sandbox status
 */
class PluginHealthWidget : public juce::Component {
public:
    PluginHealthWidget();
    ~PluginHealthWidget() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Plugin list
    [[nodiscard]] int getPluginCount() const;
    void setSelectedIndex(int index);
    [[nodiscard]] int getSelectedIndex() const;

    // Filters
    void setShowSandboxedOnly(bool show);
    [[nodiscard]] bool isShowingSandboxedOnly() const;

    // Actions
    void refresh();
    void sandboxSelected();
    void suspendSelected();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHealthWidget)
};

}  // namespace daw::ui::plugins

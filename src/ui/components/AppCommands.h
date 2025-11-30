#pragma once

#include "CommandPalette.h"
#include <vector>
#include <memory>

namespace daw::ui
{

/**
 * @brief Application command registry
 *
 * Central registry for all commands in the DAW.
 * Provides commands to CommandPalette and handles execution.
 * Follows DAW_DEV_RULES: unified input model, discoverable shortcuts.
 */
class AppCommands
{
public:
    AppCommands();
    ~AppCommands() = default;

    /**
     * @brief Get all registered commands for command palette
     * Commands will have their onExecute wired to the executor
     */
    [[nodiscard]] std::vector<daw::ui::components::CommandItem> getAllCommands() const;

    /**
     * @brief Register a command
     */
    void registerCommand(const daw::ui::components::CommandItem& command);

    /**
     * @brief Execute a command by ID
     */
    [[nodiscard]] bool executeCommand(const juce::String& commandId) const;

    /**
     * @brief Set command executor callback
     */
    void setCommandExecutor(std::function<bool(const juce::String&)> executor);

private:
    std::vector<daw::ui::components::CommandItem> commands;
    std::function<bool(const juce::String&)> commandExecutor;
};

} // namespace daw::ui


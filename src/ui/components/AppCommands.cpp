#include "AppCommands.h"

namespace daw::ui
{

AppCommands::AppCommands()
{
    // Commands will be registered with executor callbacks from MainView
    // These are placeholder commands - the executor will be set by MainView
}

std::vector<daw::ui::components::CommandItem> AppCommands::getAllCommands() const
{
    // Wire executor to each command's onExecute
    std::vector<daw::ui::components::CommandItem> result;
    result.reserve(commands.size());

    for (const auto& cmd : commands)
    {
        auto newCmd = cmd;
        if (!newCmd.onExecute && commandExecutor)
        {
            const auto id = cmd.id; // Capture by value
            newCmd.onExecute = [this, id]() {
                (void)executeCommand(id);
            };
        }
        result.push_back(newCmd);
    }
    return result;
}

void AppCommands::registerCommand(const daw::ui::components::CommandItem& command)
{
    commands.push_back(command);
}

bool AppCommands::executeCommand(const juce::String& commandId) const
{
    if (commandExecutor)
    {
        return commandExecutor(commandId);
    }
    return false;
}

void AppCommands::setCommandExecutor(std::function<bool(const juce::String&)> executor)
{
    commandExecutor = std::move(executor);
}

} // namespace daw::ui


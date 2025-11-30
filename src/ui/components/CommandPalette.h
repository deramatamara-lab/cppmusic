#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::components
{

struct CommandItem
{
    juce::String id;
    juce::String title;
    juce::String subtitle; // optional
    juce::String shortcut; // optional (display only)
    std::function<void()> onExecute; // must be safe to call on message thread
};

class ICommandProvider
{
public:
    virtual ~ICommandProvider() = default;

    // Implementations should run on a worker thread if heavy and honour cancelFlag
    virtual void queryAsync(const juce::String& term,
                            int limit,
                            std::function<void(std::vector<CommandItem>)> completion,
                            std::shared_ptr<std::atomic_bool> cancelFlag) = 0;
};

// ---------------- Fuzzy Match (fzy-like, simplified) -----------------
namespace fuzzy
{
struct MatchSpan { int start{}, length{}; };

static inline bool isWordStart(const juce::String& s, int idx)
{
    if (idx <= 0 || idx >= s.length()) return true;
    auto c = s[idx];
    auto p = s[idx-1];
    return juce::CharacterFunctions::isLetterOrDigit(c) && !juce::CharacterFunctions::isLetterOrDigit(p);
}

struct Scored
{
    float score{ -1e9f };
    juce::String text;
    CommandItem item;
    juce::Array<int> positions; // matched indices
};

float scoreMatch(const juce::String& needle, const juce::String& hay, juce::Array<int>& outPos);
std::vector<Scored> rank(const juce::String& needle, const std::vector<CommandItem>& items, int limit);

} // namespace fuzzy

// -------------- Basic Provider (in-memory, thread-pooled) --------------
class StaticCommandProvider : public ICommandProvider
{
public:
    explicit StaticCommandProvider(std::vector<CommandItem> items);
    void queryAsync(const juce::String& term, int limit,
                    std::function<void(std::vector<CommandItem>)> completion,
                    std::shared_ptr<std::atomic_bool> cancelFlag) override;

private:
    class QueryJob;
    std::vector<CommandItem> commands;
    juce::ThreadPool pool { 1 };
};

// ---------------- Row component for results -----------------
class CommandRow : public juce::Component
{
public:
    void update(const CommandItem* item, bool sel);
    void paint(juce::Graphics& g) override;

private:
    const CommandItem* cmd = nullptr;
    bool selected = false;
};

// ---------------- CommandPalette (overlay) -----------------
class CommandPalette : public juce::Component,
                       private juce::ListBoxModel,
                       private juce::TextEditor::Listener,
                       private juce::Timer
{
public:
    CommandPalette();
    ~CommandPalette() override = default;

    void setProvider(std::unique_ptr<ICommandProvider> p);
    void setCommands(std::vector<CommandItem> items);

    void showModal(juce::Component* parent);
    void hide();

    // ---- ListBoxModel ----
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    // ---- Component ----
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& k) override;

private:
    // ---- Query lifecycle ----
    void textEditorTextChanged(juce::TextEditor& te) override;
    void timerCallback() override;
    void runQuery(const juce::String& term);
    void cancelCurrent();
    void updateResults(std::vector<CommandItem> r);
    void selectRow(int r);
    void execute(int r);
    int proportional(float f) const;

    juce::TextEditor search;
    juce::ListBox list;
    CommandRow rowComp;
    std::vector<CommandItem> results;
    std::unique_ptr<ICommandProvider> provider;
    juce::String lastTerm;
    std::shared_ptr<std::atomic_bool> cancelFlag;
};

} // namespace daw::ui::components


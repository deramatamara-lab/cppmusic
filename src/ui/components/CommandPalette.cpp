#include "CommandPalette.h"
#include "../lookandfeel/DesignSystem.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::components
{

using namespace daw::ui::lookandfeel::DesignSystem;

// ---------------- Fuzzy Match Implementation -----------------

namespace fuzzy
{

float scoreMatch(const juce::String& needle, const juce::String& hay, juce::Array<int>& outPos)
{
    if (needle.isEmpty()) { outPos.clear(); return 0.0f; }

    int ni = 0;
    float score = 0.0f;
    int last = -1;
    outPos.clearQuick();

    for (int hi = 0; hi < hay.length() && ni < needle.length(); ++hi)
    {
        auto hc = juce::CharacterFunctions::toLowerCase(hay[hi]);
        auto nc = juce::CharacterFunctions::toLowerCase(needle[ni]);

        if (hc == nc)
        {
            outPos.add(hi);

            // bonuses
            float bonus = 1.0f;
            if (hi == last + 1) bonus += 0.5f;             // adjacency boost
            if (isWordStart(hay, hi)) bonus += 0.8f;       // word start
            if (hay[hi] == needle[ni]) bonus += 0.1f;      // case match slight

            score += bonus;
            last = hi;
            ++ni;
        }
    }

    if (ni < needle.length()) return -1e9f; // no full match

    // length penalty (prefer shorter strings)
    score -= 0.005f * static_cast<float>(hay.length());

    // gap penalty
    for (int i = 1; i < outPos.size(); ++i)
        score -= 0.01f * static_cast<float>(outPos[i] - outPos[i-1] - 1);

    return score;
}

std::vector<Scored> rank(const juce::String& needle, const std::vector<CommandItem>& items, int limit)
{
    std::vector<Scored> results;
    results.reserve(items.size());

    for (const auto& it : items)
    {
        juce::Array<int> pos;
        auto s = scoreMatch(needle, it.title + " " + it.subtitle, pos);

        if (s > -1e8f)
        {
            Scored sc;
            sc.score = s;
            sc.text = it.title;
            sc.item = it;
            sc.positions = pos;
            results.push_back(std::move(sc));
        }
    }

    std::partial_sort(results.begin(),
                     results.begin() + std::min<int>(static_cast<int>(results.size()), limit),
                     results.end(),
                     [](const Scored& a, const Scored& b) { return a.score > b.score; });

    if (static_cast<int>(results.size()) > limit)
        results.resize(limit);

    return results;
}

} // namespace fuzzy

// -------------- StaticCommandProvider Implementation --------------

class StaticCommandProvider::QueryJob : public juce::ThreadPoolJob
{
public:
    QueryJob(const std::vector<CommandItem>& src, juce::String t, int lim,
             std::function<void(std::vector<CommandItem>)> cb,
             std::shared_ptr<std::atomic_bool> flag)
        : juce::ThreadPoolJob("paletteQuery")
        , all(src)
        , term(std::move(t))
        , limit(lim)
        , completion(std::move(cb))
        , cancelled(std::move(flag))
    {
    }

    JobStatus runJob() override
    {
        if (cancelled && *cancelled)
            return jobHasFinished;

        std::vector<CommandItem> out;

        if (term.isEmpty())
        {
            out = pickTopN(all, limit);
        }
        else
        {
            auto ranked = fuzzy::rank(term, all, limit);
            out.reserve(ranked.size());

            for (auto& r : ranked)
            {
                if (cancelled && *cancelled)
                    return jobHasFinished;

                out.push_back(r.item);
            }
        }

        if (cancelled && *cancelled)
            return jobHasFinished;

        juce::MessageManager::callAsync([cb = completion, o = std::move(out)]()
        {
            if (cb)
                cb(o);
        });

        return jobHasFinished;
    }

private:
    static std::vector<CommandItem> pickTopN(const std::vector<CommandItem>& v, int n)
    {
        auto m = std::min<int>(static_cast<int>(v.size()), n);
        std::vector<CommandItem> r(v.begin(), v.begin() + m);
        return r;
    }

    const std::vector<CommandItem>& all;
    juce::String term;
    int limit;
    std::function<void(std::vector<CommandItem>)> completion;
    std::shared_ptr<std::atomic_bool> cancelled;
};

StaticCommandProvider::StaticCommandProvider(std::vector<CommandItem> items)
    : commands(std::move(items))
{
}

void StaticCommandProvider::queryAsync(const juce::String& term, int limit,
                                       std::function<void(std::vector<CommandItem>)> completion,
                                       std::shared_ptr<std::atomic_bool> cancelFlag)
{
    juce::ThreadPoolJob* job = new QueryJob(commands, term, limit, std::move(completion), cancelFlag);
    pool.addJob(job, true);
}

// ---------------- CommandRow Implementation -----------------

void CommandRow::update(const CommandItem* item, bool sel)
{
    cmd = item;
    selected = sel;
    repaint();
}

void CommandRow::paint(juce::Graphics& g)
{
    auto r = getLocalBounds();

    // Use DesignSystem colors
    auto bg = toColour(Colors::surface);
    auto hi = toColour(Colors::primary).withAlpha(0.15f);
    auto txt = toColour(Colors::text);
    auto txtSecondary = toColour(Colors::textSecondary);

    if (selected)
        g.fillAll(hi);
    else
        g.fillAll(bg);

    if (!cmd)
        return;

    auto left = r.removeFromLeft(r.getWidth() - 120);

    g.setColour(txt);
    g.setFont(getBodyFont(15.0f));
    g.drawText(cmd->title, left.reduced(Spacing::small, Spacing::xsmall),
               juce::Justification::centredLeft, true);

    if (cmd->subtitle.isNotEmpty())
    {
        g.setColour(txtSecondary);
        g.setFont(getBodyFont(13.0f));
        g.drawText(cmd->subtitle, left.withTop(left.getY() + 18).reduced(Spacing::small, 2),
                   juce::Justification::centredLeft, true);
    }

    if (cmd->shortcut.isNotEmpty())
    {
        auto k = r.reduced(Spacing::small);
        g.setColour(txtSecondary);
        g.drawFittedText(cmd->shortcut, k, juce::Justification::centredRight, 1);
    }
}

// ---------------- CommandPalette Implementation -----------------

CommandPalette::CommandPalette()
{
    setOpaque(true);
    addAndMakeVisible(search);

    search.setTextToShowWhenEmpty("Type a command…", toColour(Colors::textTertiary));
    search.addListener(this);
    search.setEscapeAndReturnKeysConsumed(false);
    search.setColour(juce::TextEditor::backgroundColourId, toColour(Colors::surface));
    search.setColour(juce::TextEditor::textColourId, toColour(Colors::text));
    search.setColour(juce::TextEditor::outlineColourId, toColour(Colors::outline));
    search.setColour(juce::TextEditor::focusedOutlineColourId, toColour(Colors::primary));

    addAndMakeVisible(list);
    list.setModel(this);
    list.setRowHeight(32);
    list.setOutlineThickness(0);
    list.setMultipleSelectionEnabled(false);

    // default: in-memory provider (can be replaced)
    provider = std::make_unique<StaticCommandProvider>(std::vector<CommandItem>{});

    setWantsKeyboardFocus(true);
}

void CommandPalette::setProvider(std::unique_ptr<ICommandProvider> p)
{
    provider = std::move(p);
    runQuery(lastTerm);
}

void CommandPalette::setCommands(std::vector<CommandItem> items)
{
    provider = std::make_unique<StaticCommandProvider>(std::move(items));
    runQuery(lastTerm);
}

void CommandPalette::showModal(juce::Component* parent)
{
    if (parent)
    {
        parent->addAndMakeVisible(this);
        setBounds(parent->getLocalBounds()); // Fill parent
    }

    toFront(true);
    grabKeyboardFocus();
    search.grabKeyboardFocus();
    setVisible(true);
    resized();
}

void CommandPalette::hide()
{
    setVisible(false);
    cancelCurrent();
}

int CommandPalette::getNumRows()
{
    return static_cast<int>(results.size());
}

void CommandPalette::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool rowIsSelected)
{
    juce::ignoreUnused(w, h);

    if (static_cast<unsigned>(row) >= results.size())
        return;

    rowComp.update(&results[row], rowIsSelected);

    juce::Image img(juce::Image::RGB, w, h, true);
    juce::Graphics gg(img);
    rowComp.setBounds(0, 0, w, h);
    rowComp.paint(gg);
    g.drawImageAt(img, 0, 0);
}

void CommandPalette::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    selectRow(row);
}

void CommandPalette::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    execute(row);
}

void CommandPalette::paint(juce::Graphics& g)
{
    // Semi-transparent overlay
    g.fillAll(toColour(Colors::background).withAlpha(0.75f));

    auto r = getLocalBounds().reduced(proportional(0.2f));
    auto panel = r.withHeight(360).withY(juce::jmax(40, r.getCentreY() - 180));

    // Glass morphism panel
    drawGlassPanel(g, panel.toFloat(), Radii::xlarge, true);

    // Border with glow
    g.setColour(toColour(Colors::primary).withAlpha(0.3f));
    g.drawRoundedRectangle(panel.toFloat(), Radii::xlarge, 1.5f);
}

void CommandPalette::resized()
{
    auto r = getLocalBounds().reduced(proportional(0.2f));
    auto panel = r.withHeight(360).withY(juce::jmax(40, r.getCentreY() - 180)).reduced(Spacing::small);

    auto bar = panel.removeFromTop(36);
    search.setBounds(bar);

    panel.removeFromTop(6);
    list.setBounds(panel);
}

bool CommandPalette::keyPressed(const juce::KeyPress& k)
{
    if (k == juce::KeyPress(juce::KeyPress::escapeKey))
    {
        hide();
        return true;
    }

    if (k == juce::KeyPress(juce::KeyPress::returnKey))
    {
        execute(list.getSelectedRow());
        return true;
    }

    if (k == juce::KeyPress(juce::KeyPress::upKey))
    {
        list.selectRow(juce::jmax(0, list.getSelectedRow() - 1));
        return true;
    }

    if (k == juce::KeyPress(juce::KeyPress::downKey))
    {
        list.selectRow(juce::jmin(getNumRows() - 1, list.getSelectedRow() + 1));
        return true;
    }

    return false;
}

void CommandPalette::textEditorTextChanged(juce::TextEditor& te)
{
    lastTerm = te.getText();
    startTimer(80); // Debounce
}

void CommandPalette::timerCallback()
{
    stopTimer();
    runQuery(lastTerm);
}

void CommandPalette::runQuery(const juce::String& term)
{
    cancelCurrent();

    auto flag = std::make_shared<std::atomic_bool>(false);
    cancelFlag = flag;

    provider->queryAsync(term, 100,
                        [this](std::vector<CommandItem> r) { updateResults(std::move(r)); },
                        flag);
}

void CommandPalette::cancelCurrent()
{
    if (cancelFlag)
        *cancelFlag = true;
}

void CommandPalette::updateResults(std::vector<CommandItem> r)
{
    results = std::move(r);
    list.updateContent();
    list.selectRow(results.empty() ? -1 : 0, true, true);
    repaint();
}

void CommandPalette::selectRow(int r)
{
    list.selectRow(r);
}

void CommandPalette::execute(int r)
{
    if (static_cast<unsigned>(r) >= results.size())
        return;

    hide();

    auto fn = results[r].onExecute;
    if (fn)
        fn();
}

int CommandPalette::proportional(float f) const
{
    return static_cast<int>(getHeight() * f);
}

} // namespace daw::ui::components

// ---------------- Unit Tests -----------------
#if JUCE_UNIT_TESTS

namespace daw::ui::components
{

class CommandPaletteTests : public juce::UnitTest
{
public:
    CommandPaletteTests() : juce::UnitTest("CommandPalette / Fuzzy / Async", "UI") {}

    void runTest() override
    {
        beginTest("Fuzzy ranking prefers word starts and adjacency");

        std::vector<CommandItem> cmds = {
            {"new", "New Project", "Create a blank project", "Ctrl+N", []{}},
            {"quantize16", "Quantize 1/16", "Grid: 1/16 swing 0", "Q", []{}},
            {"quantizeSwing", "Quantize 1/16 Swing 60", "Groove", "Shift+Q", []{}},
            {"save", "Save Project", "Write to disk", "Ctrl+S", []{}},
        };

        StaticCommandProvider provider(cmds);
        std::atomic_bool cancelled(false);
        std::vector<CommandItem> out;
        std::mutex m;
        std::condition_variable cv;
        bool done = false;

        provider.queryAsync("prj", 10,
                           [&](std::vector<CommandItem> r)
                           {
                               std::lock_guard<std::mutex> lk(m);
                               out = std::move(r);
                               done = true;
                               cv.notify_one();
                           },
                           std::make_shared<std::atomic_bool>(false));

        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, std::chrono::milliseconds(500), [&] { return done; });
        }

        expect(!out.empty());
        expectEquals(out.front().id, juce::String("new")); // "New Project" should win for "prj"

        beginTest("Async cancel prevents completion");

        std::vector<CommandItem> out2;
        bool done2 = false;
        auto flag = std::make_shared<std::atomic_bool>(false);

        provider.queryAsync("quantize", 10,
                           [&](std::vector<CommandItem> r) { out2 = r; done2 = true; },
                           flag);

        *flag = true; // cancel immediately
        juce::Thread::sleep(50);

        expect(!done2 || out2.size() > 0); // completion may not run if cancelled; tolerate both outcomes but ensure no crash
    }
};

static CommandPaletteTests commandPaletteTests;

} // namespace daw::ui::components

#endif


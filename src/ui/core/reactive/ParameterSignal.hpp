/**
 * @file ParameterSignal.hpp
 * @brief Specialized signals for DAW parameters, notes, and mixer channels
 */
#pragma once

#include "Signal.hpp"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace daw::ui::reactive
{

/**
 * @brief DAW parameter with metadata
 */
struct Parameter
{
    std::string id;
    std::string name;
    float value{0.0f};
    float minValue{0.0f};
    float maxValue{1.0f};
    float defaultValue{0.5f};
    std::string unit;
    bool isAutomatable{true};

    [[nodiscard]] float getNormalized() const
    {
        if (maxValue <= minValue) return 0.0f;
        return (value - minValue) / (maxValue - minValue);
    }

    void setNormalized(float norm)
    {
        value = minValue + norm * (maxValue - minValue);
    }
    
    bool operator==(const Parameter& other) const
    {
        return id == other.id && name == other.name && 
               std::abs(value - other.value) < 0.0001f;
    }
};

/**
 * @brief Signal specialized for DAW parameters
 */
class ParameterSignal : public Signal<Parameter>
{
public:
    explicit ParameterSignal(const std::string& id, const std::string& name,
                             float minVal = 0.0f, float maxVal = 1.0f, float defaultVal = 0.5f)
        : Signal<Parameter>(Parameter{id, name, defaultVal, minVal, maxVal, defaultVal, "", true})
    {}

    /**
     * @brief Set value directly (will be clamped)
     */
    void setValue(float val)
    {
        update([val](Parameter& p) {
            p.value = std::clamp(val, p.minValue, p.maxValue);
        });
    }

    /**
     * @brief Set normalized value (0-1)
     */
    void setNormalized(float norm)
    {
        update([norm](Parameter& p) {
            p.setNormalized(std::clamp(norm, 0.0f, 1.0f));
        });
    }

    /**
     * @brief Get current value
     */
    [[nodiscard]] float getValue() const { return get().value; }

    /**
     * @brief Get normalized value
     */
    [[nodiscard]] float getNormalized() const { return get().getNormalized(); }
};

/**
 * @brief MIDI note event for piano roll
 */
struct NoteEvent
{
    uint32_t id{0};           // Unique ID
    int pitch{60};            // MIDI note (0-127)
    double startBeats{0.0};   // Start position in beats
    double lengthBeats{1.0};  // Duration in beats
    float velocity{0.8f};     // Velocity (0-1)
    bool selected{false};     // Selection state
    bool muted{false};        // Mute state

    [[nodiscard]] double endBeats() const { return startBeats + lengthBeats; }
};

/**
 * @brief Collection signal for efficient note updates
 */
class NoteCollectionSignal : public SignalBase
{
public:
    using Callback = std::function<void(const std::vector<NoteEvent>&)>;

    NoteCollectionSignal() = default;

    /**
     * @brief Get all notes
     */
    [[nodiscard]] const std::vector<NoteEvent>& getNotes() const { return notes_; }

    /**
     * @brief Add a note
     */
    void addNote(const NoteEvent& note)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingNotes_.push_back(note);
        if (pendingNotes_.back().id == 0) {
            pendingNotes_.back().id = nextId_++;
        }
        dirty_ = true;
    }

    /**
     * @brief Remove a note by ID
     */
    void removeNote(uint32_t noteId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingRemovals_.push_back(noteId);
        dirty_ = true;
    }

    /**
     * @brief Update a note
     */
    void updateNote(const NoteEvent& note)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingUpdates_.push_back(note);
        dirty_ = true;
    }

    /**
     * @brief Clear all notes
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingClear_ = true;
        dirty_ = true;
    }

    /**
     * @brief Subscribe to collection changes
     */
    [[nodiscard]] Subscription subscribe(Callback callback)
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        auto id = subscriberNextId_++;
        subscribers_.push_back({id, std::move(callback)});
        
        return Subscription([this, id]() {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            subscribers_.erase(
                std::remove_if(subscribers_.begin(), subscribers_.end(),
                    [id](const auto& sub) { return sub.id == id; }),
                subscribers_.end());
        });
    }

    /**
     * @brief Flush pending changes
     */
    void flush() override
    {
        if (!dirty_) return;

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (pendingClear_) {
                notes_.clear();
                pendingClear_ = false;
                changed = true;
            }

            // Apply removals
            for (auto id : pendingRemovals_) {
                auto it = std::find_if(notes_.begin(), notes_.end(),
                    [id](const NoteEvent& n) { return n.id == id; });
                if (it != notes_.end()) {
                    notes_.erase(it);
                    changed = true;
                }
            }
            pendingRemovals_.clear();

            // Apply updates
            for (const auto& update : pendingUpdates_) {
                auto it = std::find_if(notes_.begin(), notes_.end(),
                    [&update](const NoteEvent& n) { return n.id == update.id; });
                if (it != notes_.end()) {
                    *it = update;
                    changed = true;
                }
            }
            pendingUpdates_.clear();

            // Apply additions
            for (auto& note : pendingNotes_) {
                notes_.push_back(std::move(note));
                changed = true;
            }
            pendingNotes_.clear();

            dirty_ = false;
        }

        if (changed) {
            notifySubscribers();
        }
    }

    [[nodiscard]] bool isDirty() const override { return dirty_; }

    [[nodiscard]] std::size_t getSubscriberCount() const override
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        return subscribers_.size();
    }

    /**
     * @brief Get notes in visible range (for virtualization)
     */
    [[nodiscard]] std::vector<NoteEvent> getVisibleNotes(double startBeat, double endBeat,
                                                          int minPitch = 0, int maxPitch = 127) const
    {
        std::vector<NoteEvent> visible;
        visible.reserve(notes_.size() / 4);  // Estimate 25% visible
        
        for (const auto& note : notes_) {
            if (note.endBeats() >= startBeat && note.startBeats <= endBeat &&
                note.pitch >= minPitch && note.pitch <= maxPitch) {
                visible.push_back(note);
            }
        }
        return visible;
    }

    /**
     * @brief Get note count
     */
    [[nodiscard]] std::size_t size() const { return notes_.size(); }

private:
    void notifySubscribers()
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        for (const auto& sub : subscribers_) {
            if (sub.callback) {
                sub.callback(notes_);
            }
        }
    }

    struct SubscriberEntry
    {
        uint64_t id;
        Callback callback;
    };

    std::vector<NoteEvent> notes_;
    std::vector<NoteEvent> pendingNotes_;
    std::vector<NoteEvent> pendingUpdates_;
    std::vector<uint32_t> pendingRemovals_;
    bool pendingClear_{false};
    std::atomic<bool> dirty_{false};
    mutable std::mutex mutex_;

    std::vector<SubscriberEntry> subscribers_;
    uint64_t subscriberNextId_{0};
    mutable std::mutex subscriberMutex_;
    uint32_t nextId_{1};
};

/**
 * @brief Mixer channel state
 */
struct MixerChannelState
{
    uint32_t id{0};
    std::string name;
    float volume{0.8f};      // 0-1
    float pan{0.5f};         // 0-1 (0=L, 0.5=C, 1=R)
    float peakL{0.0f};       // Peak level left
    float peakR{0.0f};       // Peak level right
    float rmsL{0.0f};        // RMS level left
    float rmsR{0.0f};        // RMS level right
    bool muted{false};
    bool soloed{false};
    bool armed{false};
};

/**
 * @brief Collection signal for mixer channels
 */
class MixerChannelCollectionSignal : public SignalBase
{
public:
    using Callback = std::function<void(const std::vector<MixerChannelState>&)>;

    MixerChannelCollectionSignal() = default;

    [[nodiscard]] const std::vector<MixerChannelState>& getChannels() const { return channels_; }

    void addChannel(const MixerChannelState& channel)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channels_.push_back(channel);
        if (channels_.back().id == 0) {
            channels_.back().id = nextId_++;
        }
        dirty_ = true;
    }

    void updateChannel(uint32_t id, const MixerChannelState& state)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(channels_.begin(), channels_.end(),
            [id](const MixerChannelState& ch) { return ch.id == id; });
        if (it != channels_.end()) {
            *it = state;
            dirty_ = true;
        }
    }

    void updateMeterLevels(uint32_t id, float peakL, float peakR, float rmsL, float rmsR)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(channels_.begin(), channels_.end(),
            [id](const MixerChannelState& ch) { return ch.id == id; });
        if (it != channels_.end()) {
            it->peakL = peakL;
            it->peakR = peakR;
            it->rmsL = rmsL;
            it->rmsR = rmsR;
            dirty_ = true;
        }
    }

    [[nodiscard]] Subscription subscribe(Callback callback)
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        auto id = subscriberNextId_++;
        subscribers_.push_back({id, std::move(callback)});
        
        return Subscription([this, id]() {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            subscribers_.erase(
                std::remove_if(subscribers_.begin(), subscribers_.end(),
                    [id](const auto& sub) { return sub.id == id; }),
                subscribers_.end());
        });
    }

    void flush() override
    {
        if (!dirty_) return;
        dirty_ = false;
        
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        for (const auto& sub : subscribers_) {
            if (sub.callback) {
                sub.callback(channels_);
            }
        }
    }

    [[nodiscard]] bool isDirty() const override { return dirty_; }
    [[nodiscard]] std::size_t getSubscriberCount() const override
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        return subscribers_.size();
    }
    [[nodiscard]] std::size_t size() const { return channels_.size(); }

private:
    struct SubscriberEntry
    {
        uint64_t id;
        Callback callback;
    };

    std::vector<MixerChannelState> channels_;
    std::atomic<bool> dirty_{false};
    mutable std::mutex mutex_;
    
    std::vector<SubscriberEntry> subscribers_;
    uint64_t subscriberNextId_{0};
    mutable std::mutex subscriberMutex_;
    uint32_t nextId_{1};
};

/**
 * @brief Pattern clip for playlist
 */
struct PatternClip
{
    uint32_t id{0};
    uint32_t patternId{0};
    std::string name;
    int trackIndex{0};
    double startBeats{0.0};
    double lengthBeats{4.0};
    float color[3]{0.3f, 0.5f, 0.7f};
    bool selected{false};
    bool muted{false};

    [[nodiscard]] double endBeats() const { return startBeats + lengthBeats; }
};

/**
 * @brief Collection signal for pattern clips
 */
class PatternClipCollectionSignal : public SignalBase
{
public:
    using Callback = std::function<void(const std::vector<PatternClip>&)>;

    [[nodiscard]] const std::vector<PatternClip>& getClips() const { return clips_; }

    void addClip(const PatternClip& clip)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clips_.push_back(clip);
        if (clips_.back().id == 0) {
            clips_.back().id = nextId_++;
        }
        dirty_ = true;
    }

    void removeClip(uint32_t clipId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clips_.erase(
            std::remove_if(clips_.begin(), clips_.end(),
                [clipId](const PatternClip& c) { return c.id == clipId; }),
            clips_.end());
        dirty_ = true;
    }

    void updateClip(const PatternClip& clip)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(clips_.begin(), clips_.end(),
            [&clip](const PatternClip& c) { return c.id == clip.id; });
        if (it != clips_.end()) {
            *it = clip;
            dirty_ = true;
        }
    }

    [[nodiscard]] std::vector<PatternClip> getVisibleClips(double startBeat, double endBeat,
                                                           int minTrack, int maxTrack) const
    {
        std::vector<PatternClip> visible;
        for (const auto& clip : clips_) {
            if (clip.endBeats() >= startBeat && clip.startBeats <= endBeat &&
                clip.trackIndex >= minTrack && clip.trackIndex <= maxTrack) {
                visible.push_back(clip);
            }
        }
        return visible;
    }

    [[nodiscard]] Subscription subscribe(Callback callback)
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        auto id = subscriberNextId_++;
        subscribers_.push_back({id, std::move(callback)});
        
        return Subscription([this, id]() {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            subscribers_.erase(
                std::remove_if(subscribers_.begin(), subscribers_.end(),
                    [id](const auto& sub) { return sub.id == id; }),
                subscribers_.end());
        });
    }

    void flush() override
    {
        if (!dirty_) return;
        dirty_ = false;
        
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        for (const auto& sub : subscribers_) {
            if (sub.callback) {
                sub.callback(clips_);
            }
        }
    }

    [[nodiscard]] bool isDirty() const override { return dirty_; }
    [[nodiscard]] std::size_t getSubscriberCount() const override
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        return subscribers_.size();
    }
    [[nodiscard]] std::size_t size() const { return clips_.size(); }

private:
    struct SubscriberEntry
    {
        uint64_t id;
        Callback callback;
    };

    std::vector<PatternClip> clips_;
    std::atomic<bool> dirty_{false};
    mutable std::mutex mutex_;
    
    std::vector<SubscriberEntry> subscribers_;
    uint64_t subscriberNextId_{0};
    mutable std::mutex subscriberMutex_;
    uint32_t nextId_{1};
};

} // namespace daw::ui::reactive

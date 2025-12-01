/**
 * @file InputRouter.hpp
 * @brief Semantic action dispatch with gesture layering and smart snapping
 */
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace daw::ui::input
{

/**
 * @brief Modifier key flags
 */
enum class Modifier : uint8_t
{
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3  // Win/Cmd key
};

inline Modifier operator|(Modifier a, Modifier b)
{
    return static_cast<Modifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline Modifier operator&(Modifier a, Modifier b)
{
    return static_cast<Modifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool hasModifier(Modifier flags, Modifier check)
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(check)) != 0;
}

/**
 * @brief Mouse button types
 */
enum class MouseButton : uint8_t
{
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3
};

/**
 * @brief Semantic action types for DAW operations
 */
enum class ActionType
{
    // Note operations
    DragNote,
    ResizeNoteStart,
    ResizeNoteEnd,
    DrawNote,
    EraseNote,
    SelectNote,
    MultiSelectNotes,
    DeselectAll,
    
    // Velocity operations
    AdjustVelocity,
    DrawVelocityRamp,
    
    // Clip operations
    DragClip,
    ResizeClipStart,
    ResizeClipEnd,
    SplitClip,
    DuplicateClip,
    
    // View operations
    PanView,
    ZoomHorizontal,
    ZoomVertical,
    ZoomRect,
    
    // Selection
    SelectRect,
    SelectLasso,
    SelectAdd,
    SelectToggle,
    
    // Transport
    SetPlayhead,
    SetLoopStart,
    SetLoopEnd,
    
    // Mixer
    AdjustFader,
    AdjustPan,
    ToggleMute,
    ToggleSolo,
    
    // Generic
    ContextMenu,
    DoubleClick,
    
    // Custom
    Custom
};

/**
 * @brief Input action with context
 */
struct InputAction
{
    ActionType type{ActionType::Custom};
    std::string customAction;  // For custom action types
    
    // Position info
    float x{0.0f};
    float y{0.0f};
    float deltaX{0.0f};
    float deltaY{0.0f};
    
    // Modifiers
    Modifier modifiers{Modifier::None};
    MouseButton button{MouseButton::None};
    
    // Target info
    uint32_t targetId{0};
    int targetIndex{-1};
    
    // Value for continuous actions
    float value{0.0f};
    
    // Time info (beats)
    double beatPosition{0.0};
    double beatDelta{0.0};
    int pitchPosition{0};
};

/**
 * @brief Gesture state machine for complex input sequences
 */
enum class GestureState
{
    Idle,
    Pressing,
    Dragging,
    Holding
};

/**
 * @brief Active gesture tracking
 */
struct ActiveGesture
{
    GestureState state{GestureState::Idle};
    ActionType action{ActionType::Custom};
    float startX{0.0f};
    float startY{0.0f};
    float currentX{0.0f};
    float currentY{0.0f};
    Modifier modifiers{Modifier::None};
    MouseButton button{MouseButton::None};
    uint32_t targetId{0};
    int targetIndex{-1};
    double startBeat{0.0};
    int startPitch{0};
    float startValue{0.0f};
};

/**
 * @brief Snap settings for grid alignment
 */
struct SnapSettings
{
    bool enabled{true};
    int division{4};           // 1 = whole, 4 = quarter, 8 = eighth, etc.
    bool triplet{false};       // Enable triplet grid
    bool magnetic{true};       // Snap to nearest vs quantize
    float magneticRadius{10.0f}; // Pixels for magnetic snap
    
    // Custom snap points
    std::vector<double> customSnapPoints;
    bool useCustomPoints{false};
    
    /**
     * @brief Get snap interval in beats
     */
    [[nodiscard]] double getSnapInterval(double beatsPerBar = 4.0) const
    {
        double interval = beatsPerBar / static_cast<double>(division);
        if (triplet) {
            interval *= 2.0 / 3.0;
        }
        return interval;
    }
    
    /**
     * @brief Snap a beat position to grid
     */
    [[nodiscard]] double snapBeat(double beat, double beatsPerBar = 4.0) const
    {
        if (!enabled) return beat;
        
        double interval = getSnapInterval(beatsPerBar);
        return std::round(beat / interval) * interval;
    }
};

/**
 * @brief Action handler callback
 */
using ActionHandler = std::function<bool(const InputAction&)>;

/**
 * @brief Input router with semantic action dispatch
 */
class InputRouter
{
public:
    InputRouter() = default;
    ~InputRouter() = default;

    /**
     * @brief Register action handler
     * @param action Action type to handle
     * @param handler Handler function (return true if consumed)
     */
    void registerHandler(ActionType action, ActionHandler handler)
    {
        handlers_[action] = std::move(handler);
    }

    /**
     * @brief Register custom action handler
     */
    void registerCustomHandler(const std::string& actionName, ActionHandler handler)
    {
        customHandlers_[actionName] = std::move(handler);
    }

    /**
     * @brief Dispatch an action to registered handlers
     * @return true if action was handled
     */
    bool dispatch(const InputAction& action)
    {
        // Check custom handlers first
        if (action.type == ActionType::Custom && !action.customAction.empty()) {
            auto it = customHandlers_.find(action.customAction);
            if (it != customHandlers_.end() && it->second) {
                return it->second(action);
            }
        }
        
        // Check type handlers
        auto it = handlers_.find(action.type);
        if (it != handlers_.end() && it->second) {
            return it->second(action);
        }
        
        return false;
    }

    /**
     * @brief Begin tracking a gesture
     */
    void beginGesture(const ActiveGesture& gesture)
    {
        activeGesture_ = gesture;
        activeGesture_.state = GestureState::Pressing;
    }

    /**
     * @brief Update active gesture
     */
    void updateGesture(float x, float y, Modifier mods)
    {
        if (activeGesture_.state == GestureState::Idle) return;
        
        float dx = x - activeGesture_.startX;
        float dy = y - activeGesture_.startY;
        
        // Transition to dragging if moved beyond threshold
        if (activeGesture_.state == GestureState::Pressing) {
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > dragThreshold_) {
                activeGesture_.state = GestureState::Dragging;
            }
        }
        
        activeGesture_.currentX = x;
        activeGesture_.currentY = y;
        activeGesture_.modifiers = mods;
    }

    /**
     * @brief End current gesture
     */
    void endGesture()
    {
        activeGesture_.state = GestureState::Idle;
    }

    /**
     * @brief Get current gesture state
     */
    [[nodiscard]] const ActiveGesture& getActiveGesture() const { return activeGesture_; }

    /**
     * @brief Check if gesture is active
     */
    [[nodiscard]] bool isGestureActive() const
    {
        return activeGesture_.state != GestureState::Idle;
    }

    /**
     * @brief Get snap settings
     */
    [[nodiscard]] SnapSettings& getSnapSettings() { return snapSettings_; }
    [[nodiscard]] const SnapSettings& getSnapSettings() const { return snapSettings_; }

    /**
     * @brief Set drag threshold in pixels
     */
    void setDragThreshold(float pixels) { dragThreshold_ = pixels; }

    /**
     * @brief Determine action from context
     * 
     * Maps raw input to semantic actions based on modifiers and context
     */
    [[nodiscard]] ActionType determineAction(
        MouseButton button,
        Modifier modifiers,
        bool onNote,
        bool onClip,
        bool onMeter,
        bool nearEdge) const
    {
        // Context menu
        if (button == MouseButton::Right) {
            return ActionType::ContextMenu;
        }
        
        // Pan view with middle mouse or Alt+Left
        if (button == MouseButton::Middle ||
            (button == MouseButton::Left && hasModifier(modifiers, Modifier::Alt))) {
            return ActionType::PanView;
        }
        
        // Zoom with Ctrl+scroll (handled elsewhere)
        
        // Multi-select with Shift
        if (button == MouseButton::Left && hasModifier(modifiers, Modifier::Shift)) {
            if (onNote) return ActionType::MultiSelectNotes;
            return ActionType::SelectRect;
        }
        
        // Toggle selection with Ctrl
        if (button == MouseButton::Left && hasModifier(modifiers, Modifier::Ctrl)) {
            if (onNote) return ActionType::SelectToggle;
        }
        
        // Note operations
        if (button == MouseButton::Left) {
            if (onNote) {
                if (nearEdge) return ActionType::ResizeNoteEnd;
                return ActionType::DragNote;
            }
            if (onClip) {
                if (nearEdge) return ActionType::ResizeClipEnd;
                return ActionType::DragClip;
            }
            if (onMeter) {
                return ActionType::AdjustFader;
            }
            
            // Drawing on empty space
            return ActionType::DrawNote;
        }
        
        return ActionType::Custom;
    }

private:
    std::unordered_map<ActionType, ActionHandler> handlers_;
    std::unordered_map<std::string, ActionHandler> customHandlers_;
    ActiveGesture activeGesture_;
    SnapSettings snapSettings_;
    float dragThreshold_{5.0f};
};

/**
 * @brief Global input router instance
 */
inline InputRouter& getGlobalInputRouter()
{
    static InputRouter instance;
    return instance;
}

} // namespace daw::ui::input

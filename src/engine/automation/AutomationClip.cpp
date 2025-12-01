/**
 * @file AutomationClip.cpp
 * @brief Implementation of the automation clip with hierarchical layers.
 */

#include "AutomationClip.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cppmusic::engine::automation {

namespace {

/**
 * @brief Smooth step function for fade in/out.
 */
float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief Linear interpolation.
 */
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

/**
 * @brief Cubic bezier interpolation.
 */
float bezier(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    return mt3 * p0 + 3.0f * mt2 * t * p1 + 3.0f * mt * t2 * p2 + t3 * p3;
}

/**
 * @brief Simple hash combine function.
 */
std::uint64_t hashCombine(std::uint64_t seed, std::uint64_t value) {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

} // anonymous namespace

float OverrideRegion::getBlendFactor(double beat) const noexcept {
    if (beat < startBeat || beat > endBeat) {
        return 0.0f;
    }
    
    // Fade in region
    if (beat < startBeat + fadeInBeats && fadeInBeats > 0.0) {
        float t = static_cast<float>((beat - startBeat) / fadeInBeats);
        return smoothstep(0.0f, 1.0f, t);
    }
    
    // Fade out region
    if (beat > endBeat - fadeOutBeats && fadeOutBeats > 0.0) {
        float t = static_cast<float>((beat - (endBeat - fadeOutBeats)) / fadeOutBeats);
        return 1.0f - smoothstep(0.0f, 1.0f, t);
    }
    
    return 1.0f;
}

float OverrideRegion::evaluate(double beat) const noexcept {
    if (points.empty()) {
        return 0.5f;  // Default center value
    }
    
    // Find surrounding points
    auto it = std::lower_bound(points.begin(), points.end(), beat,
        [](const AutomationPoint& p, double b) { return p.beat < b; });
    
    if (it == points.begin()) {
        return points.front().value;
    }
    if (it == points.end()) {
        return points.back().value;
    }
    
    const auto& p2 = *it;
    const auto& p1 = *std::prev(it);
    
    double range = p2.beat - p1.beat;
    if (range <= 0.0) {
        return p1.value;
    }
    
    float t = static_cast<float>((beat - p1.beat) / range);
    
    // Linear interpolation for override regions
    return lerp(p1.value, p2.value, t);
}

AutomationClip::AutomationClip() = default;
AutomationClip::~AutomationClip() = default;

AutomationClip::AutomationClip(const AutomationClip& other)
    : basePoints_(other.basePoints_)
    , overrides_(other.overrides_)
    , macroTransform_(other.macroTransform_)
    , macroEnabled_(other.macroEnabled_) {
}

AutomationClip& AutomationClip::operator=(const AutomationClip& other) {
    if (this != &other) {
        basePoints_ = other.basePoints_;
        overrides_ = other.overrides_;
        macroTransform_ = other.macroTransform_;
        macroEnabled_ = other.macroEnabled_;
    }
    return *this;
}

void AutomationClip::addPoint(const AutomationPoint& point) {
    basePoints_.push_back(point);
    std::sort(basePoints_.begin(), basePoints_.end());
}

bool AutomationClip::removePoint(std::size_t index) {
    if (index >= basePoints_.size()) {
        return false;
    }
    basePoints_.erase(basePoints_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void AutomationClip::clearPoints() {
    basePoints_.clear();
}

const std::vector<AutomationPoint>& AutomationClip::getPoints() const noexcept {
    return basePoints_;
}

std::size_t AutomationClip::getPointCount() const noexcept {
    return basePoints_.size();
}

void AutomationClip::addOverride(const OverrideRegion& region) {
    overrides_.push_back(region);
}

bool AutomationClip::removeOverride(std::size_t index) {
    if (index >= overrides_.size()) {
        return false;
    }
    overrides_.erase(overrides_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void AutomationClip::clearOverrides() {
    overrides_.clear();
}

const std::vector<OverrideRegion>& AutomationClip::getOverrides() const noexcept {
    return overrides_;
}

void AutomationClip::setMacroTransform(const MacroTransform& transform) {
    macroTransform_ = transform;
}

const MacroTransform& AutomationClip::getMacroTransform() const noexcept {
    return macroTransform_;
}

void AutomationClip::setMacroEnabled(bool enabled) {
    macroEnabled_ = enabled;
}

bool AutomationClip::isMacroEnabled() const noexcept {
    return macroEnabled_;
}

float AutomationClip::interpolate(const AutomationPoint& p1,
                                   const AutomationPoint& p2,
                                   double beat) const noexcept {
    double range = p2.beat - p1.beat;
    if (range <= 0.0) {
        return p1.value;
    }
    
    float t = static_cast<float>((beat - p1.beat) / range);
    
    switch (p1.curveToNext) {
        case CurveType::Step:
            return p1.value;
            
        case CurveType::Linear:
            return lerp(p1.value, p2.value, t);
            
        case CurveType::Bezier:
            if (p1.handles) {
                float cp1 = p1.value + p1.handles->outTangentY;
                float cp2 = p2.value - (p2.value - p1.value) * (1.0f - p1.handles->inTangentY);
                return bezier(p1.value, cp1, cp2, p2.value, t);
            }
            return lerp(p1.value, p2.value, t);
            
        case CurveType::SCurve:
            return lerp(p1.value, p2.value, smoothstep(0.0f, 1.0f, t));
    }
    
    return lerp(p1.value, p2.value, t);
}

float AutomationClip::evaluateBase(double beat) const noexcept {
    if (basePoints_.empty()) {
        return 0.5f;  // Default center value
    }
    
    // Find surrounding points
    auto it = std::lower_bound(basePoints_.begin(), basePoints_.end(), beat,
        [](const AutomationPoint& p, double b) { return p.beat < b; });
    
    if (it == basePoints_.begin()) {
        return basePoints_.front().value;
    }
    if (it == basePoints_.end()) {
        return basePoints_.back().value;
    }
    
    const auto& p2 = *it;
    const auto& p1 = *std::prev(it);
    
    return interpolate(p1, p2, beat);
}

float AutomationClip::evaluate(double beat) const noexcept {
    // 1. Start with base layer
    float value = evaluateBase(beat);
    
    // 2. Apply override layers
    for (const auto& override : overrides_) {
        if (override.isActiveAt(beat)) {
            float blendFactor = override.getBlendFactor(beat);
            float overrideValue = override.evaluate(beat);
            value = lerp(value, overrideValue, blendFactor);
        }
    }
    
    // 3. Apply macro transformation
    if (macroEnabled_) {
        value = macroTransform_.transform(value);
    }
    
    // Clamp to valid range
    return std::clamp(value, 0.0f, 1.0f);
}

std::vector<std::uint8_t> AutomationClip::serialize() const {
    std::vector<std::uint8_t> data;
    
    // Simple binary format (stub)
    // In real implementation, use proper serialization library
    
    // Write point count
    std::uint32_t pointCount = static_cast<std::uint32_t>(basePoints_.size());
    data.resize(sizeof(pointCount));
    std::memcpy(data.data(), &pointCount, sizeof(pointCount));
    
    // Write points
    for (const auto& point : basePoints_) {
        std::size_t offset = data.size();
        data.resize(offset + sizeof(double) + sizeof(float) + sizeof(CurveType));
        std::memcpy(data.data() + offset, &point.beat, sizeof(double));
        std::memcpy(data.data() + offset + sizeof(double), &point.value, sizeof(float));
        std::memcpy(data.data() + offset + sizeof(double) + sizeof(float), 
                    &point.curveToNext, sizeof(CurveType));
    }
    
    return data;
}

AutomationClip AutomationClip::deserialize(const std::vector<std::uint8_t>& data) {
    AutomationClip clip;
    
    if (data.size() < sizeof(std::uint32_t)) {
        return clip;
    }
    
    std::uint32_t pointCount = 0;
    std::memcpy(&pointCount, data.data(), sizeof(pointCount));
    
    std::size_t offset = sizeof(pointCount);
    for (std::uint32_t i = 0; i < pointCount; ++i) {
        if (offset + sizeof(double) + sizeof(float) + sizeof(CurveType) > data.size()) {
            break;
        }
        
        AutomationPoint point;
        std::memcpy(&point.beat, data.data() + offset, sizeof(double));
        std::memcpy(&point.value, data.data() + offset + sizeof(double), sizeof(float));
        std::memcpy(&point.curveToNext, 
                    data.data() + offset + sizeof(double) + sizeof(float),
                    sizeof(CurveType));
        
        clip.addPoint(point);
        offset += sizeof(double) + sizeof(float) + sizeof(CurveType);
    }
    
    return clip;
}

std::uint64_t AutomationClip::computeHash() const noexcept {
    std::uint64_t hash = 0;
    
    // Hash base points
    for (const auto& point : basePoints_) {
        std::uint64_t beatBits = 0;
        std::memcpy(&beatBits, &point.beat, sizeof(double));
        hash = hashCombine(hash, beatBits);
        
        std::uint32_t valueBits = 0;
        std::memcpy(&valueBits, &point.value, sizeof(float));
        hash = hashCombine(hash, valueBits);
        
        hash = hashCombine(hash, static_cast<std::uint64_t>(point.curveToNext));
    }
    
    // Hash macro transform
    if (macroEnabled_) {
        std::uint32_t scaleBits = 0;
        std::memcpy(&scaleBits, &macroTransform_.scale, sizeof(float));
        hash = hashCombine(hash, scaleBits);
        
        std::uint32_t offsetBits = 0;
        std::memcpy(&offsetBits, &macroTransform_.offset, sizeof(float));
        hash = hashCombine(hash, offsetBits);
    }
    
    return hash;
}

} // namespace cppmusic::engine::automation

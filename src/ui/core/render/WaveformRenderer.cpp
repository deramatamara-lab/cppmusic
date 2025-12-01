/**
 * @file WaveformRenderer.cpp
 * @brief GPU-accelerated waveform rendering implementation
 */

#include "WaveformRenderer.hpp"
#include "imgui.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace daw::ui::render
{

// ============================================================================
// WaveformRenderer Implementation
// ============================================================================

WaveformRenderer::WaveformRenderer() = default;

WaveformRenderer::~WaveformRenderer()
{
    shutdown();
}

bool WaveformRenderer::initialize()
{
    if (initialized_) return true;

    // Start worker thread
    running_ = true;
    workerThread_ = std::thread(&WaveformRenderer::workerThreadFunc, this);

    // GPU initialization would go here (OpenGL shader creation)
    // For now, we use the ImGui fallback
    gpuAvailable_ = false;
    initialized_ = true;
    
    return true;
}

void WaveformRenderer::shutdown()
{
    running_ = false;
    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    // Clean up GPU resources
    if (shader_ != 0) {
        // glDeleteProgram(shader_);
        shader_ = 0;
    }
    if (vao_ != 0) {
        // glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        // glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    clearCache();
    initialized_ = false;
}

std::future<std::shared_ptr<WaveformData>> WaveformRenderer::generateAsync(
    const std::string& id,
    const std::vector<float>& samples,
    int sampleRate,
    int channels)
{
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = cache_.find(id);
        if (it != cache_.end() && it->second->ready) {
            std::promise<std::shared_ptr<WaveformData>> promise;
            promise.set_value(it->second);
            return promise.get_future();
        }
    }

    // Queue task
    WaveformTask task;
    task.id = id;
    task.samples = samples;
    task.sampleRate = sampleRate;
    task.channels = channels;
    
    auto future = task.promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        pendingTasks_.push_back(std::move(task));
    }
    
    return future;
}

void WaveformRenderer::workerThreadFunc()
{
    while (running_) {
        WaveformTask task;
        bool hasTask = false;
        
        {
            std::lock_guard<std::mutex> lock(taskMutex_);
            if (!pendingTasks_.empty()) {
                task = std::move(pendingTasks_.back());
                pendingTasks_.pop_back();
                hasTask = true;
            }
        }
        
        if (hasTask) {
            auto data = std::make_shared<WaveformData>();
            data->sourceId = task.id;
            data->sampleRate = task.sampleRate;
            data->channels = task.channels;
            data->durationSeconds = static_cast<double>(task.samples.size()) /
                                    (task.sampleRate * task.channels);
            
            generateMipmaps(*data, task.samples, task.channels);
            data->ready = true;
            
            // Cache result
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                cache_[task.id] = data;
            }
            
            task.promise.set_value(data);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void WaveformRenderer::generateMipmaps(
    WaveformData& data,
    const std::vector<float>& samples,
    int channels)
{
    if (samples.empty()) return;

    // Calculate total sample frames
    std::size_t totalFrames = samples.size() / static_cast<std::size_t>(channels);
    
    // Generate mipmap levels: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
    std::vector<int> reductionFactors = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    
    for (int factor : reductionFactors) {
        std::size_t outputSize = (totalFrames + static_cast<std::size_t>(factor) - 1) /
                                 static_cast<std::size_t>(factor);
        if (outputSize < 2) break;
        
        WaveformMipmap mipmap;
        mipmap.samplesPerPixel = factor;
        mipmap.minPeaks.resize(outputSize);
        mipmap.maxPeaks.resize(outputSize);
        mipmap.rms.resize(outputSize);
        
        bool hasClipping = false;
        
        for (std::size_t i = 0; i < outputSize; ++i) {
            std::size_t startFrame = i * static_cast<std::size_t>(factor);
            std::size_t endFrame = std::min(startFrame + static_cast<std::size_t>(factor),
                                            totalFrames);
            
            float minVal = 1.0f;
            float maxVal = -1.0f;
            float sumSq = 0.0f;
            int count = 0;
            
            for (std::size_t f = startFrame; f < endFrame; ++f) {
                // Mix channels for display
                float sample = 0.0f;
                for (int ch = 0; ch < channels; ++ch) {
                    sample += samples[f * static_cast<std::size_t>(channels) +
                                      static_cast<std::size_t>(ch)];
                }
                sample /= static_cast<float>(channels);
                
                minVal = std::min(minVal, sample);
                maxVal = std::max(maxVal, sample);
                sumSq += sample * sample;
                count++;
                
                if (std::abs(sample) > 0.99f) {
                    hasClipping = true;
                }
            }
            
            mipmap.minPeaks[i] = minVal;
            mipmap.maxPeaks[i] = maxVal;
            mipmap.rms[i] = count > 0 ? std::sqrt(sumSq / static_cast<float>(count)) : 0.0f;
        }
        
        mipmap.hasClipping = hasClipping;
        data.mipmaps.push_back(std::move(mipmap));
    }
}

void WaveformRenderer::render(
    const WaveformData& /*data*/,
    float /*x*/, float /*y*/,
    float /*width*/, float /*height*/,
    int64_t /*startSample*/, int64_t /*endSample*/)
{
    // GPU rendering path (not implemented yet)
    // Falls back to ImGui rendering
}

void WaveformRenderer::renderImGui(
    void* drawListPtr,
    const WaveformData& data,
    float x, float y,
    float width, float height,
    int64_t startSample, int64_t endSample)
{
    if (!data.ready || data.mipmaps.empty()) return;
    if (width <= 0 || height <= 0) return;
    
    auto* drawList = static_cast<ImDrawList*>(drawListPtr);
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Calculate samples per pixel
    int64_t sampleRange = endSample - startSample;
    if (sampleRange <= 0) return;
    
    int samplesPerPixel = static_cast<int>(sampleRange / static_cast<int64_t>(width));
    samplesPerPixel = std::max(1, samplesPerPixel);
    
    // Get appropriate mipmap
    const WaveformMipmap* mipmap = data.getMipmapForScale(samplesPerPixel);
    if (!mipmap) return;
    
    // Draw background
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
        colors_.backgroundColor[0], colors_.backgroundColor[1],
        colors_.backgroundColor[2], colors_.backgroundColor[3]));
    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), bgColor);
    
    // Draw center line
    ImU32 centerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
        colors_.centerLine[0], colors_.centerLine[1],
        colors_.centerLine[2], colors_.centerLine[3]));
    float centerY = y + height * 0.5f;
    drawList->AddLine(ImVec2(x, centerY), ImVec2(x + width, centerY), centerColor);
    
    // Colors
    ImU32 peakColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
        colors_.peakColor[0], colors_.peakColor[1],
        colors_.peakColor[2], colors_.peakColor[3]));
    ImU32 rmsColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
        colors_.rmsColor[0], colors_.rmsColor[1],
        colors_.rmsColor[2], colors_.rmsColor[3]));
    
    // Map sample range to mipmap indices
    int64_t mipmapStartIdx = (startSample / mipmap->samplesPerPixel);
    int64_t mipmapPixels = sampleRange / mipmap->samplesPerPixel;
    
    // Draw waveform
    float halfHeight = height * 0.5f;
    float prevMinY = centerY;
    float prevMaxY = centerY;
    
    int64_t mipmapSize = static_cast<int64_t>(mipmap->minPeaks.size());
    
    for (int px = 0; px < static_cast<int>(width); ++px) {
        int64_t idx = mipmapStartIdx + (px * mipmapPixels / static_cast<int64_t>(width));
        if (idx < 0 || idx >= mipmapSize) continue;
        
        float minPeak = mipmap->minPeaks[static_cast<std::size_t>(idx)];
        float maxPeak = mipmap->maxPeaks[static_cast<std::size_t>(idx)];
        float rms = mipmap->rms[static_cast<std::size_t>(idx)];
        
        float minY = centerY - minPeak * halfHeight;
        float maxY = centerY - maxPeak * halfHeight;
        float rmsTop = centerY - rms * halfHeight;
        float rmsBot = centerY + rms * halfHeight;
        
        float xPos = x + static_cast<float>(px);
        
        // Draw based on mode
        if (mode_ == WaveformMode::Peak || mode_ == WaveformMode::PeakAndRMS) {
            // Draw peak envelope
            drawList->AddLine(ImVec2(xPos, maxY), ImVec2(xPos, minY), peakColor);
            
            // Connect to previous column for smooth appearance
            if (px > 0) {
                drawList->AddLine(ImVec2(xPos - 1, prevMaxY), ImVec2(xPos, maxY), peakColor);
                drawList->AddLine(ImVec2(xPos - 1, prevMinY), ImVec2(xPos, minY), peakColor);
            }
        }
        
        if (mode_ == WaveformMode::RMS || mode_ == WaveformMode::PeakAndRMS) {
            // Draw RMS
            drawList->AddLine(ImVec2(xPos, rmsTop), ImVec2(xPos, rmsBot), rmsColor);
        }
        
        prevMinY = minY;
        prevMaxY = maxY;
        
        stats_.verticesRendered += 4;
    }
    
    stats_.drawCalls++;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    stats_.lastRenderTimeMs = std::chrono::duration<double, std::milli>(
        endTime - startTime).count();
}

std::shared_ptr<WaveformData> WaveformRenderer::getCached(const std::string& id) const
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = cache_.find(id);
    return (it != cache_.end()) ? it->second : nullptr;
}

void WaveformRenderer::removeCached(const std::string& id)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    cache_.erase(id);
}

void WaveformRenderer::clearCache()
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    cache_.clear();
}

// ============================================================================
// MeterRenderer Implementation
// ============================================================================

void MeterRenderer::ensureMeter(std::size_t index)
{
    if (index >= meters_.size()) {
        meters_.resize(index + 1);
    }
}

void MeterRenderer::updateMeter(std::size_t index, float peakL, float peakR,
                                 float rmsL, float rmsR, float deltaTime)
{
    ensureMeter(index);
    auto& meter = meters_[index];
    
    // Update peaks with falloff
    if (peakL > meter.peakL) {
        meter.peakL = peakL;
    } else {
        meter.peakL = std::max(0.0f, meter.peakL - peakFalloff * deltaTime);
    }
    
    if (peakR > meter.peakR) {
        meter.peakR = peakR;
    } else {
        meter.peakR = std::max(0.0f, meter.peakR - peakFalloff * deltaTime);
    }
    
    // Update RMS with falloff
    if (rmsL > meter.rmsL) {
        meter.rmsL = rmsL;
    } else {
        meter.rmsL = std::max(0.0f, meter.rmsL - rmsFalloff * deltaTime);
    }
    
    if (rmsR > meter.rmsR) {
        meter.rmsR = rmsR;
    } else {
        meter.rmsR = std::max(0.0f, meter.rmsR - rmsFalloff * deltaTime);
    }
    
    // Update peak hold
    if (peakL >= meter.peakHoldL) {
        meter.peakHoldL = peakL;
        meter.peakHoldTimerL = peakHoldTime;
    } else {
        meter.peakHoldTimerL -= deltaTime;
        if (meter.peakHoldTimerL <= 0) {
            meter.peakHoldL = std::max(0.0f, meter.peakHoldL - peakFalloff * deltaTime);
        }
    }
    
    if (peakR >= meter.peakHoldR) {
        meter.peakHoldR = peakR;
        meter.peakHoldTimerR = peakHoldTime;
    } else {
        meter.peakHoldTimerR -= deltaTime;
        if (meter.peakHoldTimerR <= 0) {
            meter.peakHoldR = std::max(0.0f, meter.peakHoldR - peakFalloff * deltaTime);
        }
    }
    
    // Clip indicators
    meter.clipL = peakL > 0.99f || meter.clipL;
    meter.clipR = peakR > 0.99f || meter.clipR;
}

void MeterRenderer::renderImGui(
    void* drawListPtr,
    std::size_t index,
    float x, float y,
    float width, float height,
    bool stereo)
{
    if (index >= meters_.size()) return;
    
    const auto& meter = meters_[index];
    
    if (stereo) {
        float halfWidth = width * 0.45f;
        float gap = width * 0.1f;
        
        switch (style_) {
        case Style::Gradient:
            drawGradientMeter(drawListPtr, meter, x, y, halfWidth, height, false);
            drawGradientMeter(drawListPtr, meter, x + halfWidth + gap, y, halfWidth, height, true);
            break;
        case Style::LED:
            drawLEDMeter(drawListPtr, meter, x, y, halfWidth, height, false);
            drawLEDMeter(drawListPtr, meter, x + halfWidth + gap, y, halfWidth, height, true);
            break;
        case Style::VU:
            drawVUMeter(drawListPtr, meter, x, y, halfWidth, height, false);
            drawVUMeter(drawListPtr, meter, x + halfWidth + gap, y, halfWidth, height, true);
            break;
        }
    } else {
        switch (style_) {
        case Style::Gradient:
            drawGradientMeter(drawListPtr, meter, x, y, width, height, false);
            break;
        case Style::LED:
            drawLEDMeter(drawListPtr, meter, x, y, width, height, false);
            break;
        case Style::VU:
            drawVUMeter(drawListPtr, meter, x, y, width, height, false);
            break;
        }
    }
}

void MeterRenderer::renderBatchImGui(
    void* drawListPtr,
    float x, float y,
    float meterWidth, float meterHeight,
    float spacing,
    bool stereo)
{
    for (std::size_t i = 0; i < meters_.size(); ++i) {
        renderImGui(drawListPtr, i, x + static_cast<float>(i) * (meterWidth + spacing),
                    y, meterWidth, meterHeight, stereo);
    }
}

void MeterRenderer::drawGradientMeter(void* drawListPtr, const ChannelMeter& meter,
                                       float x, float y, float width, float height, bool isRight)
{
    auto* drawList = static_cast<ImDrawList*>(drawListPtr);
    
    float level = isRight ? meter.peakR : meter.peakL;
    float rms = isRight ? meter.rmsR : meter.rmsL;
    float peakHold = isRight ? meter.peakHoldR : meter.peakHoldL;
    bool clip = isRight ? meter.clipR : meter.clipL;
    
    // Background
    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height),
                           IM_COL32(20, 20, 24, 255));
    
    // Calculate fill heights (inverted - 0 at bottom, 1 at top)
    float levelHeight = height * level;
    float rmsHeight = height * rms;
    float holdY = y + height * (1.0f - peakHold);
    
    // Draw RMS (darker)
    if (rmsHeight > 0) {
        float rmsY = y + height - rmsHeight;
        
        // Green section (0-70%)
        float greenEnd = y + height * 0.3f;
        if (rmsY < greenEnd) {
            drawList->AddRectFilled(ImVec2(x, std::max(rmsY, greenEnd)),
                                   ImVec2(x + width, y + height),
                                   IM_COL32(40, 140, 60, 200));
        }
        
        // Yellow section (70-90%)
        float yellowEnd = y + height * 0.1f;
        if (rmsY < greenEnd && rmsY < y + height) {
            drawList->AddRectFilled(ImVec2(x, std::max(rmsY, yellowEnd)),
                                   ImVec2(x + width, std::min(greenEnd, y + height - rmsHeight)),
                                   IM_COL32(180, 160, 40, 200));
        }
        
        // Red section (90-100%)
        if (rmsY < yellowEnd) {
            drawList->AddRectFilled(ImVec2(x, rmsY),
                                   ImVec2(x + width, yellowEnd),
                                   IM_COL32(180, 50, 50, 200));
        }
    }
    
    // Draw peak (brighter overlay)
    if (levelHeight > 0) {
        float levelY = y + height - levelHeight;
        
        // Gradient from green to yellow to red
        ImU32 topColor = IM_COL32(200, 60, 60, 255);
        ImU32 midColor = IM_COL32(220, 180, 50, 255);
        ImU32 botColor = IM_COL32(60, 200, 80, 255);
        
        // Simple three-section fill
        float greenEnd = y + height * 0.3f;
        float yellowEnd = y + height * 0.1f;
        
        if (levelY < y + height) {
            // Green section
            drawList->AddRectFilled(ImVec2(x + 1, std::max(levelY, greenEnd)),
                                   ImVec2(x + width - 1, y + height),
                                   botColor);
        }
        if (levelY < greenEnd) {
            // Yellow section
            drawList->AddRectFilled(ImVec2(x + 1, std::max(levelY, yellowEnd)),
                                   ImVec2(x + width - 1, greenEnd),
                                   midColor);
        }
        if (levelY < yellowEnd) {
            // Red section
            drawList->AddRectFilled(ImVec2(x + 1, levelY),
                                   ImVec2(x + width - 1, yellowEnd),
                                   topColor);
        }
    }
    
    // Peak hold indicator
    if (peakHold > 0.01f) {
        ImU32 holdColor = IM_COL32(255, 255, 255, 200);
        if (peakHold > 0.9f) holdColor = IM_COL32(255, 100, 100, 255);
        drawList->AddLine(ImVec2(x, holdY), ImVec2(x + width, holdY), holdColor, 2.0f);
    }
    
    // Clip indicator
    if (clip) {
        drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + 4),
                               IM_COL32(255, 50, 50, 255));
    }
    
    // Border
    drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height),
                     IM_COL32(60, 60, 70, 255));
}

void MeterRenderer::drawLEDMeter(void* drawListPtr, const ChannelMeter& meter,
                                  float x, float y, float width, float height, bool isRight)
{
    auto* drawList = static_cast<ImDrawList*>(drawListPtr);
    
    float level = isRight ? meter.peakR : meter.peakL;
    bool clip = isRight ? meter.clipR : meter.clipL;
    
    // Background
    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height),
                           IM_COL32(20, 20, 24, 255));
    
    // LED segments
    int numSegments = 20;
    float segmentHeight = (height - 2) / static_cast<float>(numSegments);
    float gap = 1.0f;
    
    for (int i = 0; i < numSegments; ++i) {
        float segY = y + height - (static_cast<float>(i) + 1) * segmentHeight;
        float segLevel = static_cast<float>(i + 1) / static_cast<float>(numSegments);
        
        ImU32 color;
        if (i >= numSegments - 2) {
            // Red (top 2)
            color = level >= segLevel ? IM_COL32(255, 60, 60, 255) : IM_COL32(60, 20, 20, 100);
        } else if (i >= numSegments - 5) {
            // Yellow (next 3)
            color = level >= segLevel ? IM_COL32(255, 200, 60, 255) : IM_COL32(60, 50, 20, 100);
        } else {
            // Green (rest)
            color = level >= segLevel ? IM_COL32(60, 220, 80, 255) : IM_COL32(20, 50, 25, 100);
        }
        
        drawList->AddRectFilled(ImVec2(x + 1, segY + gap),
                               ImVec2(x + width - 1, segY + segmentHeight - gap),
                               color);
    }
    
    // Clip indicator
    if (clip) {
        drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + segmentHeight),
                               IM_COL32(255, 50, 50, 255));
    }
}

void MeterRenderer::drawVUMeter(void* drawListPtr, const ChannelMeter& meter,
                                 float x, float y, float width, float height, bool isRight)
{
    // VU-style meter (simplified - just draw gradient style with different colors)
    drawGradientMeter(drawListPtr, meter, x, y, width, height, isRight);
}

void MeterRenderer::resetAll()
{
    for (auto& meter : meters_) {
        meter = ChannelMeter{};
    }
}

// ============================================================================
// IconAtlas Implementation
// ============================================================================

bool IconAtlas::build(
    const std::unordered_map<std::string, std::string>& /*iconPaths*/,
    int /*size*/)
{
    // Icon atlas building would require image loading and packing
    // This is a stub for now
    ready_ = false;
    return false;
}

const IconAtlas::IconEntry* IconAtlas::getIcon(const std::string& name) const
{
    auto it = icons_.find(name);
    return (it != icons_.end()) ? &it->second : nullptr;
}

void IconAtlas::renderIcon(void* /*drawList*/, const std::string& /*name*/,
                            float /*x*/, float /*y*/, float /*size*/,
                            const float* /*tintColor*/)
{
    // Stub - would render textured quad
}

} // namespace daw::ui::render

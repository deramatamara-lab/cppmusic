#include "AudioEngine.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

namespace daw::ui::imgui
{

AudioEngine::AudioEngine()
{
    createDemoPatterns();
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

bool AudioEngine::initialize()
{
    // Initialize SDL Audio
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        std::cerr << "SDL Audio init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_AudioSpec desired{};
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_F32SYS;
    desired.channels = NUM_CHANNELS;
    desired.samples = BUFFER_SIZE;
    desired.callback = audioCallback;
    desired.userdata = this;

    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &audioSpec_, 0);
    if (audioDevice_ == 0)
    {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Calculate samples per step at current BPM
    // 1 step = 1/4 beat at 4 steps per beat
    double beatsPerSecond = bpm_.load() / 60.0;
    double stepsPerSecond = beatsPerSecond * 4.0;  // 16th notes
    samplesPerStep_ = static_cast<double>(SAMPLE_RATE) / stepsPerSecond;

    // Start audio processing (paused initially)
    SDL_PauseAudioDevice(audioDevice_, 0);

    std::cout << "Audio Engine initialized: " << SAMPLE_RATE << "Hz, "
              << BUFFER_SIZE << " samples/buffer" << std::endl;

    return true;
}

void AudioEngine::shutdown()
{
    if (audioDevice_ != 0)
    {
        SDL_CloseAudioDevice(audioDevice_);
        audioDevice_ = 0;
    }
}

void AudioEngine::play()
{
    std::cout << "AudioEngine::play() called, isPaused=" << isPaused_.load() << std::endl;

    if (!isPaused_.load())
    {
        currentStep_.store(0);
        sampleCounter_ = 0.0;

        // Trigger step 0 immediately when starting
        std::lock_guard<std::mutex> lock(patternMutex_);
        std::cout << "Triggering step 0, patterns=" << patterns_.size() << std::endl;
        triggerStep(0);
    }
    isPaused_.store(false);
    isPlaying_.store(true);
    std::cout << "AudioEngine playing, isPlaying=" << isPlaying_.load() << std::endl;
}

void AudioEngine::stop()
{
    isPlaying_.store(false);
    isPaused_.store(false);
    currentStep_.store(0);
    sampleCounter_ = 0.0;

    // Release all voices
    std::lock_guard<std::mutex> lock(patternMutex_);
    for (auto& pattern : patterns_)
    {
        for (auto& channel : pattern.channels)
        {
            channel.voice.active = false;
            channel.voice.envelope = 0.0f;
        }
    }
}

void AudioEngine::pause()
{
    isPaused_.store(true);
    isPlaying_.store(false);
}

void AudioEngine::setBPM(double bpm)
{
    bpm = std::clamp(bpm, 20.0, 999.0);
    bpm_.store(bpm);

    double beatsPerSecond = bpm / 60.0;
    double stepsPerSecond = beatsPerSecond * 4.0;
    samplesPerStep_ = static_cast<double>(SAMPLE_RATE) / stepsPerSecond;
}

double AudioEngine::getPositionBeats() const
{
    return static_cast<double>(currentStep_.load()) / 4.0;
}

void AudioEngine::setPattern(int index)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index >= 0 && index < static_cast<int>(patterns_.size()))
    {
        currentPattern_.store(index);
    }
}

Pattern& AudioEngine::getPattern(int index)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index < 0 || index >= static_cast<int>(patterns_.size()))
    {
        index = 0;
    }
    return patterns_[index];
}

void AudioEngine::addPattern()
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    Pattern p;
    p.name = "Pattern " + std::to_string(patterns_.size() + 1);

    // Add 4 default channels
    for (int i = 0; i < 4; ++i)
    {
        Channel ch;
        ch.name = (i == 0) ? "Kick" : (i == 1) ? "Snare" : (i == 2) ? "HiHat" : "Bass";
        ch.waveform = (i == 3) ? Waveform::Saw : (i == 2) ? Waveform::Noise : Waveform::Square;
        p.channels.push_back(ch);
    }
    patterns_.push_back(std::move(p));
}

void AudioEngine::setStep(int channelIdx, int step, bool active)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()) && step < 16)
        {
            pat.channels[channelIdx].steps[step].active = active;
        }
    }
}

void AudioEngine::setStepNote(int channelIdx, int step, int note)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()) && step < 16)
        {
            pat.channels[channelIdx].steps[step].note = note;
        }
    }
}

void AudioEngine::setStepVelocity(int channelIdx, int step, float velocity)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()) && step < 16)
        {
            pat.channels[channelIdx].steps[step].velocity = velocity;
        }
    }
}

bool AudioEngine::getStep(int channelIdx, int step) const
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        const auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()) && step < 16)
        {
            return pat.channels[channelIdx].steps[step].active;
        }
    }
    return false;
}

int AudioEngine::getNumChannels() const
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        return static_cast<int>(patterns_[patIdx].channels.size());
    }
    return 0;
}

std::string AudioEngine::getChannelName(int channelIdx) const
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        const auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            return pat.channels[channelIdx].name;
        }
    }
    return "Channel";
}

void AudioEngine::setChannelVolume(int channelIdx, float volume)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            pat.channels[channelIdx].volume = std::clamp(volume, 0.0f, 1.0f);
        }
    }
}

void AudioEngine::setChannelPan(int channelIdx, float pan)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            pat.channels[channelIdx].pan = std::clamp(pan, -1.0f, 1.0f);
        }
    }
}

void AudioEngine::setChannelMute(int channelIdx, bool muted)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            pat.channels[channelIdx].muted = muted;
        }
    }
}

void AudioEngine::setChannelSolo(int channelIdx, bool soloed)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            pat.channels[channelIdx].soloed = soloed;
        }
    }
}

void AudioEngine::setChannelWaveform(int channelIdx, Waveform waveform)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            pat.channels[channelIdx].waveform = waveform;
        }
    }
}

int AudioEngine::addChannel(const std::string& name)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        Channel ch;
        ch.name = name;
        pat.channels.push_back(ch);
        return static_cast<int>(pat.channels.size()) - 1;
    }
    return -1;
}

void AudioEngine::noteOn(int channelIdx, int note, float velocity)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            auto& voice = pat.channels[channelIdx].voice;
            voice.active = true;
            voice.note = note;
            voice.velocity = velocity;
            voice.phase = 0.0;
            voice.envelope = 0.0f;
            voice.envTarget = 1.0f;
            voice.envRate = 0.01f;  // Attack rate
            voice.waveform = pat.channels[channelIdx].waveform;
        }
    }
}

void AudioEngine::noteOff(int channelIdx)
{
    std::lock_guard<std::mutex> lock(patternMutex_);
    int patIdx = currentPattern_.load();
    if (patIdx < static_cast<int>(patterns_.size()))
    {
        auto& pat = patterns_[patIdx];
        if (channelIdx < static_cast<int>(pat.channels.size()))
        {
            auto& voice = pat.channels[channelIdx].voice;
            voice.envTarget = 0.0f;
            voice.envRate = 0.005f;  // Release rate
        }
    }
}

void AudioEngine::setOnStepCallback(std::function<void(int)> callback)
{
    onStepCallback_ = std::move(callback);
}

void AudioEngine::audioCallback(void* userdata, Uint8* stream, int len)
{
    auto* engine = static_cast<AudioEngine*>(userdata);
    auto* output = reinterpret_cast<float*>(stream);
    int numSamples = len / (sizeof(float) * NUM_CHANNELS);
    engine->processAudio(output, numSamples);
}

void AudioEngine::processAudio(float* output, int numSamples)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    // Clear output
    std::memset(output, 0, numSamples * NUM_CHANNELS * sizeof(float));

    float leftSum = 0.0f;
    float rightSum = 0.0f;

    // Lock for pattern access
    std::lock_guard<std::mutex> lock(patternMutex_);

    int patIdx = currentPattern_.load();
    if (patIdx >= static_cast<int>(patterns_.size()))
    {
        return;
    }
    auto& pattern = patterns_[patIdx];

    // Check for any solo
    bool anySolo = false;
    for (const auto& ch : pattern.channels)
    {
        if (ch.soloed) anySolo = true;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        // Advance sequencer if playing
        if (isPlaying_.load())
        {
            sampleCounter_ += 1.0;
            if (sampleCounter_ >= samplesPerStep_)
            {
                sampleCounter_ -= samplesPerStep_;
                int step = currentStep_.load();
                step = (step + 1) % pattern.length;
                currentStep_.store(step);

                // Trigger notes for this step
                triggerStep(step);

                // Notify UI
                if (onStepCallback_)
                {
                    onStepCallback_(step);
                }
            }
        }

        // Process all channels
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (auto& channel : pattern.channels)
        {
            // Skip muted channels or non-soloed when solo active
            if (channel.muted) continue;
            if (anySolo && !channel.soloed) continue;

            auto& voice = channel.voice;
            if (!voice.active && voice.envelope <= 0.001f) continue;

            // Update envelope
            updateEnvelope(voice);

            // Generate sample
            float sample = processVoice(voice, static_cast<double>(SAMPLE_RATE));
            sample *= voice.envelope * voice.velocity * channel.volume;

            // Pan
            float leftGain = (channel.pan <= 0.0f) ? 1.0f : (1.0f - channel.pan);
            float rightGain = (channel.pan >= 0.0f) ? 1.0f : (1.0f + channel.pan);

            sampleL += sample * leftGain;
            sampleR += sample * rightGain;

            // Deactivate voice when envelope reaches zero
            if (!voice.active && voice.envelope <= 0.001f)
            {
                voice.envelope = 0.0f;
            }
        }

        // Soft clip
        sampleL = std::tanh(sampleL);
        sampleR = std::tanh(sampleR);

        // Master volume
        constexpr float masterGain = 0.5f;
        output[i * 2] = sampleL * masterGain;
        output[i * 2 + 1] = sampleR * masterGain;

        leftSum += std::abs(sampleL);
        rightSum += std::abs(sampleR);
    }

    // Update meters
    leftLevel_.store(leftSum / static_cast<float>(numSamples));
    rightLevel_.store(rightSum / static_cast<float>(numSamples));

    // CPU usage estimate
    auto endTime = std::chrono::high_resolution_clock::now();
    double processingTime = std::chrono::duration<double>(endTime - startTime).count();
    double bufferTime = static_cast<double>(numSamples) / SAMPLE_RATE;
    cpuUsage_.store(static_cast<float>((processingTime / bufferTime) * 100.0));
}

float AudioEngine::processVoice(SynthVoice& voice, double sampleRate)
{
    // Apply pitch envelope for kick drums (drops pitch over time)
    int effectiveNote = voice.baseNote;
    if (voice.pitchEnv > 0.0f)
    {
        effectiveNote = voice.baseNote + static_cast<int>(voice.pitchEnv * 24.0f); // Up to 2 octaves
        voice.pitchEnv *= (1.0f - voice.pitchEnvDecay);
        if (voice.pitchEnv < 0.01f) voice.pitchEnv = 0.0f;
    }

    double freq = noteToFrequency(effectiveNote);
    double phaseInc = freq / sampleRate;

    float sample = 0.0f;

    switch (voice.waveform)
    {
        case Waveform::Sine:
            sample = static_cast<float>(std::sin(voice.phase * 2.0 * M_PI));
            break;

        case Waveform::Square:
            sample = (voice.phase < 0.5) ? 0.8f : -0.8f;
            break;

        case Waveform::Saw:
            sample = static_cast<float>(2.0 * voice.phase - 1.0) * 0.7f;
            break;

        case Waveform::Triangle:
            sample = static_cast<float>(4.0 * std::abs(voice.phase - 0.5) - 1.0);
            break;

        case Waveform::Noise:
            voice.rng = voice.rng * 1103515245 + 12345;
            sample = static_cast<float>((voice.rng >> 16) & 0x7FFF) / 16384.0f - 1.0f;
            // High-pass filter for snare-like sound
            sample *= 0.5f;
            break;
    }

    voice.phase += phaseInc;
    if (voice.phase >= 1.0) voice.phase -= 1.0;

    voice.sampleCount++;

    return sample;
}

void AudioEngine::triggerStep(int step)
{
    int patIdx = currentPattern_.load();
    if (patIdx >= static_cast<int>(patterns_.size())) return;

    auto& pattern = patterns_[patIdx];
    std::cout << "triggerStep(" << step << "), pattern has " << pattern.channels.size() << " channels" << std::endl;

    for (size_t chIdx = 0; chIdx < pattern.channels.size(); ++chIdx)
    {
        auto& channel = pattern.channels[chIdx];
        if (step < static_cast<int>(channel.steps.size()))
        {
            const auto& stepData = channel.steps[step];
            if (stepData.active)
            {
                std::cout << "  Channel " << chIdx << " (" << channel.name << ") step " << step
                         << " is ACTIVE, note=" << stepData.note << std::endl;
                auto& voice = channel.voice;
                voice.active = true;
                voice.baseNote = stepData.note;
                voice.note = stepData.note;
                voice.velocity = stepData.velocity;
                voice.phase = 0.0;
                voice.envelope = 0.0f;
                voice.envTarget = 1.0f;
                voice.sampleCount = 0;
                voice.waveform = channel.waveform;

                // Set up envelope based on sound type
                // Instant attack, then decay
                voice.envelope = 1.0f;  // Start at full volume immediately
                voice.envTarget = 0.0f; // Decay to zero

                if (channel.waveform == Waveform::Sine) // Kick
                {
                    voice.envRate = 0.0005f;  // Slow decay for body
                    voice.pitchEnv = 1.0f;    // Start high
                    voice.pitchEnvDecay = 0.0002f; // Fast pitch drop
                }
                else if (channel.waveform == Waveform::Noise) // Snare/HiHat
                {
                    voice.envRate = 0.002f;  // Medium decay
                    voice.pitchEnv = 0.0f;
                }
                else // Synth sounds (Saw, Square)
                {
                    voice.envRate = 0.001f;  // Longer for bass/synth
                    voice.pitchEnv = 0.0f;
                }
            }
        }
    }
}

void AudioEngine::updateEnvelope(SynthVoice& voice)
{
    if (voice.envelope < voice.envTarget)
    {
        voice.envelope += voice.envRate;
        if (voice.envelope > voice.envTarget)
            voice.envelope = voice.envTarget;
    }
    else if (voice.envelope > voice.envTarget)
    {
        voice.envelope -= voice.envRate;
        if (voice.envelope < voice.envTarget)
            voice.envelope = voice.envTarget;
    }
}

double AudioEngine::noteToFrequency(int note) const
{
    // MIDI note 69 = A4 = 440Hz
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

void AudioEngine::createDemoPatterns()
{
    patterns_.clear();

    Pattern p;
    p.name = "Pattern 1";

    // Kick channel
    Channel kick;
    kick.name = "Kick";
    kick.waveform = Waveform::Sine;
    kick.steps[0].active = true;  kick.steps[0].note = 36;
    kick.steps[4].active = true;  kick.steps[4].note = 36;
    kick.steps[8].active = true;  kick.steps[8].note = 36;
    kick.steps[12].active = true; kick.steps[12].note = 36;
    p.channels.push_back(kick);

    // Snare channel
    Channel snare;
    snare.name = "Snare";
    snare.waveform = Waveform::Noise;
    snare.steps[4].active = true;  snare.steps[4].note = 38;
    snare.steps[12].active = true; snare.steps[12].note = 38;
    p.channels.push_back(snare);

    // HiHat channel
    Channel hihat;
    hihat.name = "HiHat";
    hihat.waveform = Waveform::Noise;
    hihat.volume = 0.3f;
    for (int i = 0; i < 16; i += 2)
    {
        hihat.steps[i].active = true;
        hihat.steps[i].note = 42;
        hihat.steps[i].velocity = (i % 4 == 0) ? 0.8f : 0.5f;
    }
    p.channels.push_back(hihat);

    // Bass channel
    Channel bass;
    bass.name = "Bass";
    bass.waveform = Waveform::Saw;
    bass.steps[0].active = true;  bass.steps[0].note = 36;
    bass.steps[3].active = true;  bass.steps[3].note = 36;
    bass.steps[8].active = true;  bass.steps[8].note = 38;
    bass.steps[11].active = true; bass.steps[11].note = 36;
    p.channels.push_back(bass);

    patterns_.push_back(std::move(p));

    // Add empty Pattern 2
    Pattern p2;
    p2.name = "Pattern 2";
    for (int i = 0; i < 4; ++i)
    {
        Channel ch;
        ch.name = "Channel " + std::to_string(i + 1);
        p2.channels.push_back(ch);
    }
    patterns_.push_back(std::move(p2));
}

} // namespace daw::ui::imgui

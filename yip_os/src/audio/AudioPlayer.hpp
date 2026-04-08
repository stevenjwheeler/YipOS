#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>

namespace YipOS {

class AudioPlayer {
public:
    AudioPlayer() = default;
    ~AudioPlayer();

    // Load a WAV file (16-bit PCM, mono or stereo, any sample rate)
    bool LoadWAV(const std::string& path);

    // Load an OGG Vorbis file (any sample rate, mono or stereo)
    bool LoadOGG(const std::string& path);

    // Play the loaded WAV. If loop=true, repeats until Stop().
    void Play(bool loop = false);

    // Stop playback immediately
    void Stop();

    bool IsPlaying() const { return playing_; }
    bool IsLoaded() const { return !samples_.empty(); }

    // Volume 0.0–1.0 (default 0.3 — intentionally quiet)
    void SetVolume(float v) { volume_ = v; }
    float GetVolume() const { return volume_; }

private:
    void PlayThread();

    std::vector<int16_t> samples_;
    int sample_rate_ = 0;
    int channels_ = 0;

    std::atomic<bool> playing_{false};
    std::atomic<bool> loop_{false};
    float volume_ = 0.3f;
    std::thread play_thread_;
    std::mutex mutex_;
};

} // namespace YipOS

#ifndef _WIN32

#include "AudioPlayer.hpp"
#include "core/Logger.hpp"

#include <pulse/simple.h>
#include <pulse/error.h>

namespace YipOS {

void AudioPlayer::PlayThread() {
    pa_sample_spec spec{};
    spec.format = PA_SAMPLE_S16LE;
    spec.rate = static_cast<uint32_t>(sample_rate_);
    spec.channels = static_cast<uint8_t>(channels_);

    int err = 0;
    pa_simple* s = pa_simple_new(
        nullptr, "yip_os", PA_STREAM_PLAYBACK, nullptr,
        "notification", &spec, nullptr, nullptr, &err);

    if (!s) {
        Logger::Warning("AudioPlayer: pa_simple_new failed: " +
                        std::string(pa_strerror(err)));
        playing_ = false;
        return;
    }

    const size_t chunk_samples = static_cast<size_t>(sample_rate_) * channels_ / 10; // 100ms
    const size_t total = samples_.size();
    std::vector<int16_t> buf(chunk_samples);

    do {
        size_t pos = 0;
        while (playing_ && pos < total) {
            size_t n = std::min(chunk_samples, total - pos);
            float vol = volume_;
            for (size_t i = 0; i < n; i++)
                buf[i] = static_cast<int16_t>(samples_[pos + i] * vol);
            if (pa_simple_write(s, buf.data(),
                                n * sizeof(int16_t), &err) < 0) {
                Logger::Warning("AudioPlayer: write error: " +
                                std::string(pa_strerror(err)));
                playing_ = false;
                break;
            }
            pos += n;
        }
    } while (playing_ && loop_);

    // Drain remaining audio
    pa_simple_drain(s, nullptr);
    pa_simple_free(s);
    playing_ = false;
}

} // namespace YipOS

#endif

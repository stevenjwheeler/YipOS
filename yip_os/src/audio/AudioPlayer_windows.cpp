#ifdef _WIN32

#include "AudioPlayer.hpp"
#include "core/Logger.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace YipOS {

void AudioPlayer::PlayThread() {
    // Build a WAV in memory for PlaySound
    // RIFF header + fmt chunk + data chunk
    const uint32_t data_size = static_cast<uint32_t>(samples_.size() * sizeof(int16_t));
    const uint32_t fmt_size = 16;
    const uint32_t riff_size = 4 + (8 + fmt_size) + (8 + data_size);

    std::vector<uint8_t> wav;
    wav.reserve(12 + 8 + fmt_size + 8 + data_size);

    auto write16 = [&](uint16_t v) {
        wav.push_back(static_cast<uint8_t>(v));
        wav.push_back(static_cast<uint8_t>(v >> 8));
    };
    auto write32 = [&](uint32_t v) {
        wav.push_back(static_cast<uint8_t>(v));
        wav.push_back(static_cast<uint8_t>(v >> 8));
        wav.push_back(static_cast<uint8_t>(v >> 16));
        wav.push_back(static_cast<uint8_t>(v >> 24));
    };
    auto writeTag = [&](const char* tag) {
        wav.insert(wav.end(), tag, tag + 4);
    };

    // RIFF header
    writeTag("RIFF");
    write32(riff_size);
    writeTag("WAVE");

    // fmt chunk
    writeTag("fmt ");
    write32(fmt_size);
    write16(1); // PCM
    write16(static_cast<uint16_t>(channels_));
    write32(static_cast<uint32_t>(sample_rate_));
    write32(static_cast<uint32_t>(sample_rate_ * channels_ * 2)); // byte rate
    write16(static_cast<uint16_t>(channels_ * 2)); // block align
    write16(16); // bits per sample

    // data chunk — apply volume scaling
    writeTag("data");
    write32(data_size);
    float vol = volume_;
    for (size_t i = 0; i < samples_.size(); i++) {
        int16_t s = static_cast<int16_t>(samples_[i] * vol);
        write16(static_cast<uint16_t>(s));
    }

    do {
        PlaySoundA(reinterpret_cast<LPCSTR>(wav.data()),
                   nullptr, SND_MEMORY | SND_SYNC);
    } while (playing_ && loop_);

    playing_ = false;
}

} // namespace YipOS

#endif

#include "AudioPlayer.hpp"
#include "core/Logger.hpp"

#include <fstream>
#include <cstring>

#include "stb/stb_vorbis.c"

namespace YipOS {

AudioPlayer::~AudioPlayer() {
    Stop();
}

bool AudioPlayer::LoadWAV(const std::string& path) {
    Stop();

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        Logger::Warning("AudioPlayer: cannot open " + path);
        return false;
    }

    // Read RIFF header
    char riff[4];
    f.read(riff, 4);
    if (std::memcmp(riff, "RIFF", 4) != 0) {
        Logger::Warning("AudioPlayer: not a RIFF file: " + path);
        return false;
    }

    uint32_t file_size;
    f.read(reinterpret_cast<char*>(&file_size), 4);

    char wave[4];
    f.read(wave, 4);
    if (std::memcmp(wave, "WAVE", 4) != 0) {
        Logger::Warning("AudioPlayer: not a WAVE file: " + path);
        return false;
    }

    // Find fmt and data chunks
    int fmt_channels = 0, fmt_rate = 0, fmt_bits = 0;
    bool found_fmt = false, found_data = false;

    while (f && !found_data) {
        char chunk_id[4];
        uint32_t chunk_size;
        f.read(chunk_id, 4);
        f.read(reinterpret_cast<char*>(&chunk_size), 4);
        if (!f) break;

        if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
            uint16_t audio_fmt;
            f.read(reinterpret_cast<char*>(&audio_fmt), 2);
            if (audio_fmt != 1) {
                Logger::Warning("AudioPlayer: only PCM WAV supported");
                return false;
            }
            uint16_t ch;
            f.read(reinterpret_cast<char*>(&ch), 2);
            fmt_channels = ch;
            uint32_t rate;
            f.read(reinterpret_cast<char*>(&rate), 4);
            fmt_rate = rate;
            f.seekg(6, std::ios::cur); // byte rate + block align
            uint16_t bits;
            f.read(reinterpret_cast<char*>(&bits), 2);
            fmt_bits = bits;
            // Skip rest of fmt chunk
            if (chunk_size > 16)
                f.seekg(chunk_size - 16, std::ios::cur);
            found_fmt = true;
        } else if (std::memcmp(chunk_id, "data", 4) == 0 && found_fmt) {
            if (fmt_bits != 16) {
                Logger::Warning("AudioPlayer: only 16-bit PCM supported, got " +
                                std::to_string(fmt_bits));
                return false;
            }
            size_t sample_count = chunk_size / 2;
            samples_.resize(sample_count);
            f.read(reinterpret_cast<char*>(samples_.data()), chunk_size);
            found_data = true;
        } else {
            // Skip unknown chunk
            f.seekg(chunk_size, std::ios::cur);
        }
    }

    if (!found_data || samples_.empty()) {
        Logger::Warning("AudioPlayer: no PCM data found in " + path);
        samples_.clear();
        return false;
    }

    sample_rate_ = fmt_rate;
    channels_ = fmt_channels;
    Logger::Info("AudioPlayer: loaded " + path + " (" +
                 std::to_string(sample_rate_) + "Hz, " +
                 std::to_string(channels_) + "ch, " +
                 std::to_string(samples_.size() / channels_ / sample_rate_) + "s)");
    return true;
}

bool AudioPlayer::LoadOGG(const std::string& path) {
    Stop();

    int channels = 0, sample_rate = 0;
    short* output = nullptr;
    int sample_count = stb_vorbis_decode_filename(
        path.c_str(), &channels, &sample_rate, &output);
    if (sample_count <= 0 || !output) {
        Logger::Warning("AudioPlayer: cannot decode OGG: " + path);
        return false;
    }

    samples_.assign(output, output + sample_count * channels);
    free(output);

    sample_rate_ = sample_rate;
    channels_ = channels;
    Logger::Info("AudioPlayer: loaded " + path + " (" +
                 std::to_string(sample_rate_) + "Hz, " +
                 std::to_string(channels_) + "ch, " +
                 std::to_string(samples_.size() / channels_ / sample_rate_) + "s)");
    return true;
}

void AudioPlayer::Play(bool loop) {
    Stop();
    if (samples_.empty()) return;

    loop_ = loop;
    playing_ = true;
    play_thread_ = std::thread(&AudioPlayer::PlayThread, this);
}

void AudioPlayer::Stop() {
    playing_ = false;
    if (play_thread_.joinable())
        play_thread_.join();
}

} // namespace YipOS

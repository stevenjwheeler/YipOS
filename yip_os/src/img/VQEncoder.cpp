#define STB_IMAGE_IMPLEMENTATION
#include "VQEncoder.hpp"
#include "core/Logger.hpp"
#include "stb/stb_image.h"
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

namespace YipOS {

bool VQEncoder::LoadCodebook(const std::string& npy_path) {
    std::ifstream f(npy_path, std::ios::binary);
    if (!f) {
        Logger::Error("VQEncoder: cannot open codebook: " + npy_path);
        return false;
    }

    // Parse .npy header: magic "\x93NUMPY", major, minor, header_len, then Python dict
    char magic[6];
    f.read(magic, 6);
    if (std::memcmp(magic, "\x93NUMPY", 6) != 0) {
        Logger::Error("VQEncoder: invalid .npy magic");
        return false;
    }

    uint8_t major, minor;
    f.read(reinterpret_cast<char*>(&major), 1);
    f.read(reinterpret_cast<char*>(&minor), 1);

    uint16_t header_len;
    if (major == 1) {
        f.read(reinterpret_cast<char*>(&header_len), 2);
    } else {
        // v2: 4-byte header length
        uint32_t h4;
        f.read(reinterpret_cast<char*>(&h4), 4);
        header_len = static_cast<uint16_t>(h4);
    }

    // Read the header dict to determine dtype
    std::string header(header_len, '\0');
    f.read(header.data(), header_len);

    // Detect dtype from header: '|u1' = uint8, '<f4' = float32
    bool is_uint8 = header.find("|u1") != std::string::npos ||
                    header.find("'u1'") != std::string::npos;

    constexpr int total = VQ_ENTRIES * BLOCK_PIXELS;

    if (is_uint8) {
        // Read raw uint8 data directly
        std::vector<uint8_t> raw(total);
        f.read(reinterpret_cast<char*>(raw.data()), total);
        if (!f) {
            Logger::Error("VQEncoder: failed to read codebook data (uint8)");
            return false;
        }
        for (int i = 0; i < VQ_ENTRIES; i++) {
            for (int j = 0; j < BLOCK_PIXELS; j++) {
                codebook_[i][j] = raw[i * BLOCK_PIXELS + j] ? 1 : 0;
            }
        }
    } else {
        // Assume float32
        std::vector<float> raw(total);
        f.read(reinterpret_cast<char*>(raw.data()), total * sizeof(float));
        if (!f) {
            Logger::Error("VQEncoder: failed to read codebook data (float32)");
            return false;
        }
        for (int i = 0; i < VQ_ENTRIES; i++) {
            for (int j = 0; j < BLOCK_PIXELS; j++) {
                codebook_[i][j] = raw[i * BLOCK_PIXELS + j] > 0.5f ? 1 : 0;
            }
        }
    }

    codebook_loaded_ = true;
    Logger::Info("VQEncoder: loaded codebook from " + npy_path +
                 " (" + std::to_string(VQ_ENTRIES) + " entries)");
    return true;
}

int VQEncoder::FindNearest(const uint8_t* block) const {
    int best_idx = 0;
    int best_dist = BLOCK_PIXELS + 1;

    for (int i = 0; i < VQ_ENTRIES; i++) {
        int dist = 0;
        for (int j = 0; j < BLOCK_PIXELS; j++) {
            dist += (block[j] != codebook_[i][j]);
        }
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
            if (dist == 0) break;
        }
    }
    return best_idx;
}

bool VQEncoder::EncodeImage(const std::string& image_path, IndexGrid& out) {
    if (!codebook_loaded_) {
        Logger::Error("VQEncoder: codebook not loaded");
        return false;
    }

    // Load image as grayscale
    int w, h, channels;
    unsigned char* data = stbi_load(image_path.c_str(), &w, &h, &channels, 1);
    if (!data) {
        Logger::Error("VQEncoder: failed to load image: " + image_path);
        return false;
    }

    constexpr int target = IMG_COLS * BLOCK_SIZE; // 256

    // Resize to 256x256 with aspect ratio preservation (letterbox/pillarbox).
    // The image is scaled to fit within 256x256, centered, with black padding.
    std::vector<float> buf(target * target, 0.0f); // black background

    float scale = std::min(static_cast<float>(target) / w,
                           static_cast<float>(target) / h);
    int scaled_w = static_cast<int>(w * scale);
    int scaled_h = static_cast<int>(h * scale);
    int offset_x = (target - scaled_w) / 2;
    int offset_y = (target - scaled_h) / 2;

    for (int y = 0; y < scaled_h; y++) {
        int sy = y * h / scaled_h;
        for (int x = 0; x < scaled_w; x++) {
            int sx = x * w / scaled_w;
            buf[(y + offset_y) * target + (x + offset_x)] =
                static_cast<float>(data[sy * w + sx]);
        }
    }
    stbi_image_free(data);

    // Floyd-Steinberg dither to 1-bit
    for (int y = 0; y < target; y++) {
        for (int x = 0; x < target; x++) {
            float old_val = buf[y * target + x];
            float new_val = old_val >= 128.0f ? 255.0f : 0.0f;
            buf[y * target + x] = new_val;
            float err = old_val - new_val;

            if (x + 1 < target)
                buf[y * target + x + 1] += err * 7.0f / 16.0f;
            if (y + 1 < target) {
                if (x > 0)
                    buf[(y + 1) * target + x - 1] += err * 3.0f / 16.0f;
                buf[(y + 1) * target + x] += err * 5.0f / 16.0f;
                if (x + 1 < target)
                    buf[(y + 1) * target + x + 1] += err * 1.0f / 16.0f;
            }
        }
    }

    // Extract 8x8 blocks and find nearest codebook entry
    uint8_t block[BLOCK_PIXELS];
    for (int ty = 0; ty < IMG_ROWS; ty++) {
        for (int tx = 0; tx < IMG_COLS; tx++) {
            int y0 = ty * BLOCK_SIZE;
            int x0 = tx * BLOCK_SIZE;
            for (int by = 0; by < BLOCK_SIZE; by++) {
                for (int bx = 0; bx < BLOCK_SIZE; bx++) {
                    block[by * BLOCK_SIZE + bx] =
                        buf[(y0 + by) * target + (x0 + bx)] > 128.0f ? 1 : 0;
                }
            }
            out[ty][tx] = static_cast<uint8_t>(FindNearest(block));
        }
    }

    Logger::Info("VQEncoder: encoded " + image_path);
    return true;
}

} // namespace YipOS

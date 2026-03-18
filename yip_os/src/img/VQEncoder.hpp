#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace YipOS {

class VQEncoder {
public:
    static constexpr int IMG_COLS = 32;
    static constexpr int IMG_ROWS = 32;
    static constexpr int BLOCK_SIZE = 8;
    static constexpr int VQ_ENTRIES = 256;
    static constexpr int BLOCK_PIXELS = BLOCK_SIZE * BLOCK_SIZE; // 64

    using IndexGrid = std::array<std::array<uint8_t, IMG_COLS>, IMG_ROWS>;

    bool LoadCodebook(const std::string& npy_path);
    bool EncodeImage(const std::string& image_path, IndexGrid& out);
    bool IsLoaded() const { return codebook_loaded_; }

private:
    // Each codebook entry: 64 binary values (0 or 1)
    std::array<std::array<uint8_t, BLOCK_PIXELS>, VQ_ENTRIES> codebook_{};
    bool codebook_loaded_ = false;

    int FindNearest(const uint8_t* block) const;
};

} // namespace YipOS

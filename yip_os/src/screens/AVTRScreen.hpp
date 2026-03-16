#pragma once

#include "Screen.hpp"
#include <array>

namespace YipOS {

class AVTRScreen : public Screen {
public:
    AVTRScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

private:
    void WriteTile(int tx, int ty);
    void WriteTileLine(int tx, int ty, const char* text, int row);

    struct Tile {
        const char* line1;
        const char* line2;
        const char* screen_name;
    };

    static constexpr Tile TILES[2][3] = {
        {{"CHANGE", nullptr, "AVTR_CHANGE"}, {"CTRL", nullptr, "AVTR_CTRL"}, {"-----", nullptr, nullptr}},
        {{"-----", nullptr, nullptr},        {"-----", nullptr, nullptr},    {"-----", nullptr, nullptr}},
    };

    static constexpr int BTN_COLS[3] = {4, 20, 36};

    std::array<std::array<bool, 3>, 2> tile_highlighted_{};
};

} // namespace YipOS

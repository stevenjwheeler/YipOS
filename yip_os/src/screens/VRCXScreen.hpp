#pragma once

#include "Screen.hpp"
#include <array>

namespace YipOS {

class VRCXScreen : public Screen {
public:
    VRCXScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

private:
    void WriteTile(int tx, int ty);

    struct Tile {
        const char* label;
        const char* screen_name; // nullptr = inactive
    };

    static constexpr Tile TILES[2][3] = {
        {{"WORLDS", "VRCX_WORLDS"}, {"FEED", nullptr}, {"STATUS", nullptr}},
        {{"NOTIF", nullptr},        {"-----", nullptr}, {"-----", nullptr}},
    };

    static constexpr int BTN_COLS[3] = {4, 20, 36};

    std::array<std::array<bool, 3>, 2> tile_highlighted_{};
};

} // namespace YipOS

#pragma once

#include "Screen.hpp"
#include <string>
#include <array>

namespace YipOS {

class TEXTScreen : public Screen {
public:
    TEXTScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    void Update() override;

private:
    void ComputeCells(const std::string& text);
    void FlushDiff();       // write only changed cells
    void FlushRefresh();    // rolling refresh: write a chunk of cells per tick

    std::string last_text_;
    static constexpr int TEXT_COLS = 38; // COLS - 2 (frame borders)
    static constexpr int TEXT_ROWS = 6;  // rows 1-6
    static constexpr int TOTAL_CELLS = TEXT_COLS * TEXT_ROWS; // 228

    // Desired vs displayed cell state (char index at each position)
    std::array<int, TOTAL_CELLS> desired_{}; // what we want
    std::array<int, TOTAL_CELLS> displayed_{}; // what's on screen

    // Rolling refresh cursor
    int refresh_pos_ = 0;
    static constexpr int REFRESH_CHUNK = 12; // cells per update tick
};

} // namespace YipOS

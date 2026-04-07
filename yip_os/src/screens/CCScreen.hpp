#pragma once

#include "Screen.hpp"
#include <string>
#include <vector>
#include <array>

namespace YipOS {

class CCScreen : public Screen {
public:
    CCScreen(PDAController& pda);
    ~CCScreen() override;

    void Render() override;
    void RenderDynamic() override;
    void Update() override;
    bool OnInput(const std::string& key) override;

private:
    void WriteInverted(int col, int row, const std::string& text);
    void StartCC();
    void StopCC();
    bool FilterText(const std::string& text) const;

    // Word-wrap text into LINE_WIDTH lines, appending to output
    void WordWrap(const std::string& text, std::vector<std::string>& output);

    // Check if tentative display is enabled (ULTRA write speed only)
    bool IsTentativeEnabled() const;

    // Committed zone state
    int committed_cursor_ = 1; // next row to write committed text
    int committed_first_row_ = 1;
    int committed_last_row_ = 6; // adjusted based on tentative mode

    // Pending committed lines (word-wrapped, FIFO)
    std::vector<std::string> pending_lines_;
    static constexpr int LINE_WIDTH = 38;
    static constexpr size_t MAX_PENDING_LINES = 20;
    static constexpr int LINES_PER_TICK = 3;

    static constexpr int LEFT_COL = 1;
    static constexpr int RIGHT_COL = 38; // COLS - 2

    // Tentative zone state (rows 5-6 when enabled)
    static constexpr int TENT_FIRST_ROW = 5;
    static constexpr int TENT_LAST_ROW = 6;
    uint32_t last_tentative_version_ = 0;
    // Displayed content for diff-based writes (one per tentative row)
    std::array<std::string, 2> tent_displayed_; // [0]=row5, [1]=row6

    // Write speed threshold for tentative (ULTRA = 0.02f)
    static constexpr float ULTRA_WRITE_DELAY = 0.025f;

    bool started_by_screen_ = false;

    // VRChat chatbox relay
    bool chatbox_relay_ = false;
    std::string chatbox_buffer_;
    double last_chatbox_send_ = 0;
    static constexpr double CHATBOX_INTERVAL = 3.0;
    static constexpr int CHATBOX_MAX_LEN = 144;
    void UpdateRelayIndicator();
};

} // namespace YipOS

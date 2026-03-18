#include "TEXTScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Config.hpp"
#include "core/Glyphs.hpp"
#include "net/OSCManager.hpp"

namespace YipOS {

using namespace Glyphs;

TEXTScreen::TEXTScreen(PDAController& pda) : Screen(pda) {
    name = "TEXT";
    macro_index = 28;
    update_interval = 0.25f; // check for text changes 4x/sec

    desired_.fill(' ');
    displayed_.fill(' '); // macro stamp provides space background
}

void TEXTScreen::ComputeCells(const std::string& text) {
    desired_.fill(' ');

    if (text.empty()) {
        // "No text set" / "Use Text tab in app"
        const char* line1 = "No text set";
        const char* line2 = "Use Text tab in app";
        int r1 = 2 * TEXT_COLS; // row 3 (index 2)
        int r2 = 3 * TEXT_COLS; // row 4 (index 3)
        for (int i = 0; line1[i]; i++) desired_[r1 + i + 1] = line1[i];
        for (int i = 0; line2[i]; i++) desired_[r2 + i + 1] = line2[i];
        return;
    }

    // Word-wrap text into desired_ array
    int row = 0, col = 0;
    size_t i = 0;
    while (i < text.size() && row < TEXT_ROWS) {
        if (text[i] == '\n') {
            row++;
            col = 0;
            i++;
            continue;
        }

        // Find next word
        size_t word_start = i;
        while (i < text.size() && text[i] != ' ' && text[i] != '\n') i++;
        int word_len = static_cast<int>(i - word_start);

        // Wrap if word doesn't fit (unless we're at column 0)
        if (col > 0 && col + word_len > TEXT_COLS) {
            row++;
            col = 0;
            if (row >= TEXT_ROWS) break;
        }

        // Write word characters
        for (size_t j = word_start; j < i && row < TEXT_ROWS; j++) {
            int ch = static_cast<int>(text[j]);
            if (ch < 32 || ch > 126) ch = '?';
            if (col < TEXT_COLS) {
                desired_[row * TEXT_COLS + col] = ch;
                col++;
            } else {
                row++;
                col = 0;
                if (row >= TEXT_ROWS) break;
                desired_[row * TEXT_COLS + col] = ch;
                col++;
            }
        }

        // Handle trailing space
        if (i < text.size() && text[i] == ' ') {
            if (col < TEXT_COLS) col++; // leave as space (already filled)
            i++;
        }
    }
}

void TEXTScreen::FlushDiff() {
    display_.CancelBuffered();
    display_.BeginBuffered();
    for (int idx = 0; idx < TOTAL_CELLS; idx++) {
        if (desired_[idx] != displayed_[idx]) {
            int r = idx / TEXT_COLS;
            int c = idx % TEXT_COLS;
            display_.WriteChar(1 + c, 1 + r, desired_[idx]);
            displayed_[idx] = desired_[idx];
        }
    }
}

void TEXTScreen::FlushRefresh() {
    // Write REFRESH_CHUNK cells from the rolling position
    display_.CancelBuffered();
    display_.BeginBuffered();
    for (int n = 0; n < REFRESH_CHUNK; n++) {
        int idx = refresh_pos_;
        int r = idx / TEXT_COLS;
        int c = idx % TEXT_COLS;
        display_.WriteChar(1 + c, 1 + r, desired_[idx]);
        displayed_[idx] = desired_[idx];

        refresh_pos_ = (refresh_pos_ + 1) % TOTAL_CELLS;
    }
}

void TEXTScreen::Render() {
    RenderFrame("TEXT");

    // Compute current text so we only write non-space chars
    // (macro stamp already provides the empty background)
    bool chatbox_mode = pda_.GetConfig().GetState("text.vrc_chatbox") == "1";
    std::string text;
    if (chatbox_mode) {
        OSCManager* osc = pda_.GetOSCManager();
        text = osc ? osc->GetChatboxText() : "";
    } else {
        text = pda_.GetDisplayText();
    }
    ComputeCells(text);
    last_text_ = text;

    for (int idx = 0; idx < TOTAL_CELLS; idx++) {
        if (desired_[idx] != ' ') {
            int r = idx / TEXT_COLS;
            int c = idx % TEXT_COLS;
            display_.WriteChar(1 + c, 1 + r, desired_[idx]);
        }
        displayed_[idx] = desired_[idx];
    }

    RenderStatusBar();
}

void TEXTScreen::RenderDynamic() {
    RenderClock();
    RenderCursor();
}

void TEXTScreen::Update() {
    // Determine text source: chatbox or user-typed
    bool chatbox_mode = pda_.GetConfig().GetState("text.vrc_chatbox") == "1";
    std::string text;
    if (chatbox_mode) {
        OSCManager* osc = pda_.GetOSCManager();
        text = osc ? osc->GetChatboxText() : "";
    } else {
        text = pda_.GetDisplayText();
    }

    // Recompute cells
    ComputeCells(text);

    if (text != last_text_) {
        // Text changed — diff-write only changed cells
        last_text_ = text;
        FlushDiff();
    } else {
        // Text unchanged — rolling refresh to combat packet loss
        FlushRefresh();
    }
}

} // namespace YipOS

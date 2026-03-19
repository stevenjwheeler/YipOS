#include "StonkScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/StockClient.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace YipOS {

using namespace Glyphs;

StonkScreen::StonkScreen(PDAController& pda) : Screen(pda) {
    name = "STONK";
    macro_index = 31;
    update_interval = 1.0f;
    refresh_interval = -1;

    selected_symbol_ = pda_.GetConfig().GetState("stonk.selected", "DOGE");
    time_window_ = pda_.GetConfig().GetState("stonk.window", "1MO");
}

void StonkScreen::Render() {
    SyncFromConfig();
    RenderFrame("STONK");
    RenderGraph();
    RenderInfoLine();
    RenderStatusBar();
}

void StonkScreen::RenderDynamic() {
    SyncFromConfig();
    RenderInfoLine();
    RenderGraph();
    RenderClock();
    RenderCursor();
}

void StonkScreen::SyncFromConfig() {
    selected_symbol_ = pda_.GetConfig().GetState("stonk.selected", "DOGE");
    time_window_ = pda_.GetConfig().GetState("stonk.window", "1MO");
}

void StonkScreen::RenderGraph() {
    auto* client = pda_.GetStockClient();
    if (!client) return;

    auto* quote = client->GetQuote(selected_symbol_);

    // Scale bar (cols 1-4)
    if (quote && !quote->history.empty()) {
        float hi = *std::max_element(quote->history.begin(), quote->history.end());
        float lo = *std::min_element(quote->history.begin(), quote->history.end());

        std::string hi_str = FormatScale(hi);
        std::string lo_str = FormatScale(lo);

        display_.WriteText(1, GRAPH_TOP, hi_str);
        display_.WriteText(1, GRAPH_BOTTOM, lo_str);

        // Plot graph
        if (hi == lo) hi = lo + 1.0f;  // avoid division by zero

        for (int i = 0; i < static_cast<int>(quote->history.size()) && i < GRAPH_WIDTH; i++) {
            int col = GRAPH_LEFT + i;
            float val = quote->history[i];
            float frac = (val - lo) / (hi - lo);
            int level = std::clamp(static_cast<int>(std::round(frac * GRAPH_LEVELS)), 0, GRAPH_LEVELS);

            // Clear column
            for (int r = GRAPH_TOP; r <= GRAPH_BOTTOM; r++) {
                display_.WriteChar(col, r, ' ');
            }

            // Draw dot — solid for above prev_close, shaded for below
            if (level > 0) {
                int cell = (level - 1) / 2;
                int half = (level - 1) % 2;
                int row = GRAPH_BOTTOM - cell;
                bool negative = (quote->prev_close > 0 && val < quote->prev_close);
                int glyph;
                if (half == 1) {
                    glyph = negative ? G_UPPER_SHADE : G_UPPER;
                } else {
                    glyph = negative ? G_LOWER_SHADE : G_LOWER;
                }
                display_.WriteGlyph(col, row, glyph);
            }
        }
    } else {
        // No data
        display_.WriteText(1, GRAPH_TOP, "----");
        display_.WriteText(1, GRAPH_BOTTOM, "----");
        display_.WriteText(GRAPH_LEFT + 5, 3, "Fetching data...");
    }
}

void StonkScreen::RenderInfoLine() {
    auto& d = display_;

    // Clear row 6
    for (int c = 1; c < COLS - 1; c++)
        d.WriteChar(c, 6, ' ');

    // Ticker symbol (inverted = touchable) at cols 1-5
    std::string sym = selected_symbol_;
    while (static_cast<int>(sym.size()) < 4) sym += ' ';
    if (static_cast<int>(sym.size()) > 5) sym = sym.substr(0, 5);
    for (int i = 0; i < static_cast<int>(sym.size()); i++) {
        d.WriteChar(1 + i, 6, static_cast<int>(sym[i]) + INVERT_OFFSET);
    }

    // Time window label (inverted = touchable) at cols 35-37
    std::string tw = time_window_;
    while (static_cast<int>(tw.size()) < 3) tw += ' ';
    for (int i = 0; i < static_cast<int>(tw.size()); i++) {
        d.WriteChar(35 + i, 6, static_cast<int>(tw[i]) + INVERT_OFFSET);
    }

    // Price and change
    auto* client = pda_.GetStockClient();
    if (!client) return;
    auto* quote = client->GetQuote(selected_symbol_);
    if (quote && quote->current_price > 0) {
        std::string price = FormatPrice(quote->current_price);
        std::string change = FormatChange(quote->current_price, quote->prev_close);

        // Price at cols 8-20
        if (static_cast<int>(price.size()) > 12) price = price.substr(0, 12);
        d.WriteText(8, 6, "$" + price);

        // Change at cols 22-33
        if (static_cast<int>(change.size()) > 10) change = change.substr(0, 10);
        d.WriteText(22, 6, change);
    }
}

std::string StonkScreen::FormatScale(float price) const {
    char buf[8];
    if (price >= 100000.0f) {
        std::snprintf(buf, sizeof(buf), "%3dK", static_cast<int>(price / 1000.0f));
    } else if (price >= 10000.0f) {
        std::snprintf(buf, sizeof(buf), "%.0fK", price / 1000.0f);
    } else if (price >= 1000.0f) {
        std::snprintf(buf, sizeof(buf), "%.1fK", price / 1000.0f);
    } else if (price >= 100.0f) {
        std::snprintf(buf, sizeof(buf), " %3.0f", price);
    } else if (price >= 10.0f) {
        std::snprintf(buf, sizeof(buf), "%4.1f", price);
    } else if (price >= 1.0f) {
        std::snprintf(buf, sizeof(buf), "%4.2f", price);
    } else {
        std::snprintf(buf, sizeof(buf), "%4.3f", price);
    }
    // Ensure exactly 4 chars
    std::string s(buf);
    if (static_cast<int>(s.size()) > 4) s = s.substr(0, 4);
    while (static_cast<int>(s.size()) < 4) s = " " + s;
    return s;
}

std::string StonkScreen::FormatPrice(float price) const {
    char buf[16];
    if (price >= 10000.0f) {
        std::snprintf(buf, sizeof(buf), "%.0f", price);
    } else if (price >= 100.0f) {
        std::snprintf(buf, sizeof(buf), "%.1f", price);
    } else if (price >= 1.0f) {
        std::snprintf(buf, sizeof(buf), "%.2f", price);
    } else if (price >= 0.01f) {
        std::snprintf(buf, sizeof(buf), "%.3f", price);
    } else {
        std::snprintf(buf, sizeof(buf), "%.4f", price);
    }
    return buf;
}

std::string StonkScreen::FormatChange(float current, float prev_close) const {
    if (prev_close <= 0) return "---";
    float pct = ((current - prev_close) / prev_close) * 100.0f;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%+.1f%%", pct);
    return buf;
}

void StonkScreen::CycleTimeWindow() {
    for (int i = 0; i < TIME_WINDOW_COUNT; i++) {
        if (time_window_ == TIME_WINDOWS[i]) {
            time_window_ = TIME_WINDOWS[(i + 1) % TIME_WINDOW_COUNT];
            break;
        }
    }
    pda_.GetConfig().SetState("stonk.window", time_window_);
    // Trigger re-fetch
    last_data_fetch_ = 0;
}

bool StonkScreen::OnInput(const std::string& key) {
    // Touch 13 (bottom-left area): navigate to stock list
    if (key == "13") {
        pda_.SetPendingNavigate("STONK_LIST");
        return true;
    }
    // Touch 53 (bottom-right area): cycle time window
    if (key == "53") {
        CycleTimeWindow();
        // Re-render immediately so the new time window label is visible
        // before the blocking fetch starts
        pda_.StartRender(this);
        // Fetch new data for the updated window
        auto* client = pda_.GetStockClient();
        if (client) {
            client->FetchQuote(selected_symbol_, time_window_);
        }
        return true;
    }
    // Joystick: cycle through symbols
    if (key == "Joystick") {
        auto* client = pda_.GetStockClient();
        if (!client) return true;
        auto& syms = client->GetSymbols();
        if (syms.empty()) return true;
        for (int i = 0; i < static_cast<int>(syms.size()); i++) {
            if (syms[i] == selected_symbol_) {
                selected_symbol_ = syms[(i + 1) % syms.size()];
                pda_.GetConfig().SetState("stonk.selected", selected_symbol_);
                pda_.StartRender(this);
                return true;
            }
        }
        // Symbol not found in list, pick first
        selected_symbol_ = syms[0];
        pda_.GetConfig().SetState("stonk.selected", selected_symbol_);
        pda_.StartRender(this);
        return true;
    }
    return false;
}

void StonkScreen::Update() {
    SyncFromConfig();

    auto* client = pda_.GetStockClient();
    if (!client) return;

    auto* quote = client->GetQuote(selected_symbol_);
    if (quote && quote->last_fetch != last_data_fetch_) {
        // Data has changed, redraw (info line first so it appears before graph)
        last_data_fetch_ = quote->last_fetch;
        display_.BeginBuffered();
        RenderInfoLine();
        RenderGraph();
    }
}

} // namespace YipOS

#include "BFIParamScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Config.hpp"
#include <cstdio>
#include <algorithm>
#include <string>
#include <cstring>

namespace YipOS {

using namespace Glyphs;

BFIParamScreen::BFIParamScreen(PDAController& pda) : ListScreen(pda) {
    name = "BFI_PARAM";
    macro_index = 23;

    std::string p = pda.GetConfig().GetState("bfi.param", "2");
    active_idx_ = std::clamp(std::stoi(p), 0, BFI_PARAM_COUNT - 1);

    for (int i = 0; i < BFI_PARAM_COUNT; i++) {
        int len = static_cast<int>(std::strlen(BFI_PARAMS[i].display_name));
        if (len > max_name_len_) max_name_len_ = len;
    }
}

int BFIParamScreen::ItemCount() const {
    return BFI_PARAM_COUNT;
}

void BFIParamScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= BFI_PARAM_COUNT) return;

    const char* pname = BFI_PARAMS[idx].display_name;
    int name_len = static_cast<int>(std::strlen(pname));

    bool is_active = (idx == active_idx_);

    int prefix[2] = { is_active ? '+' : ' ', ' ' };
    for (int c = 0; c < 2; c++) {
        int ch = prefix[c];
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
    for (int c = 0; c < max_name_len_; c++) {
        int ch = (c < name_len) ? static_cast<int>(pname[c]) : ' ';
        if (selected && (c + 2) < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(3 + c, row, ch);
    }
}

void BFIParamScreen::WriteSelectionMark(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= BFI_PARAM_COUNT) return;

    const char* pname = BFI_PARAMS[idx].display_name;
    bool is_active = (idx == active_idx_);
    int chars[3] = {is_active ? '+' : ' ', ' ', static_cast<int>(pname[0])};
    for (int c = 0; c < SEL_WIDTH; c++) {
        int ch = chars[c];
        if (selected) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
}

void BFIParamScreen::Render() {
    RenderFrame("BFI PARAM");
    if (BFI_PARAM_COUNT > 0) {
        RenderRows();
    }
    RenderStatusBar();
}

void BFIParamScreen::RenderDynamic() {
    if (BFI_PARAM_COUNT > 0) {
        RenderRows();
        RenderPageIndicators();
    }
    RenderClock();
    RenderCursor();
}

bool BFIParamScreen::OnSelect(int index) {
    if (index >= BFI_PARAM_COUNT) return true;

    int old_active = active_idx_;
    active_idx_ = index;
    pda_.GetConfig().SetState("bfi.param", std::to_string(index));

    // Redraw old and new active rows to move the "+" indicator
    display_.BeginPriority();
    int old_row_on_page = old_active - page_ * ROWS_PER_PAGE;
    if (old_row_on_page >= 0 && old_row_on_page < ROWS_PER_PAGE) {
        RenderRow(old_row_on_page, old_row_on_page == cursor_);
    }
    RenderRow(cursor_, true);
    display_.EndPriority();

    pda_.SetPendingNavigate("__POP__");
    return true;
}

bool BFIParamScreen::OnInput(const std::string& key) {
    return ListScreen::OnInput(key);
}

} // namespace YipOS

#include "StonkListScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/StockClient.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

StonkListScreen::StonkListScreen(PDAController& pda) : ListScreen(pda) {
    name = "STONK_LIST";
    macro_index = 32;

    auto* client = pda_.GetStockClient();
    if (client) {
        symbols_ = client->GetSymbols();
    }
}

void StonkListScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(symbols_.size())) return;

    const std::string& sym = symbols_[idx];
    std::string current_sel = pda_.GetConfig().GetState("stonk.selected", "DOGE");
    bool is_active = (sym == current_sel);

    std::string line = sym;
    if (is_active) line += " *";

    for (int c = 0; c < static_cast<int>(line.size()) && c < COLS - 2; c++) {
        int ch = static_cast<int>(line[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
}

void StonkListScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(symbols_.size())) return;

    const std::string& sym = symbols_[idx];
    for (int c = 0; c < 3 && c < static_cast<int>(sym.size()); c++) {
        int ch = static_cast<int>(sym[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool StonkListScreen::OnSelect(int index) {
    if (index < static_cast<int>(symbols_.size())) {
        pda_.GetConfig().SetState("stonk.selected", symbols_[index]);
        Logger::Info("STONK: selected " + symbols_[index]);
        pda_.SetPendingNavigate("__POP__");
    }
    return true;
}

} // namespace YipOS

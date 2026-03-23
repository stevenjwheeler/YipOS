#include "VRCXWorldsScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Logger.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

VRCXWorldsScreen::VRCXWorldsScreen(PDAController& pda) : ListScreen(pda) {
    name = "WORLDS";
    macro_index = 9;
    LoadData();
}

void VRCXWorldsScreen::LoadData() {
    auto* vrcx = pda_.GetVRCXData();
    if (vrcx && vrcx->IsOpen()) {
        worlds_ = vrcx->GetWorlds(300);
    }
}

void VRCXWorldsScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(worlds_.size())) return;

    auto& w = worlds_[idx];

    std::string dur = FormatDuration(w.time_seconds);
    int dur_len = static_cast<int>(dur.size());
    int content_width = COLS - 2;
    int name_max = content_width - dur_len - 1;
    std::string wname = w.world_name.empty() ? "(unknown)" : w.world_name;
    if (static_cast<int>(wname.size()) > name_max) {
        wname = wname.substr(0, name_max);
    }

    for (int c = 0; c < static_cast<int>(wname.size()); c++) {
        int ch = static_cast<int>(wname[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }

    int dur_col = COLS - 1 - dur_len;
    d.WriteText(dur_col, row, dur);
}

void VRCXWorldsScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(worlds_.size())) return;

    std::string wname = worlds_[idx].world_name.empty() ? "(unknown)" : worlds_[idx].world_name;
    for (int c = 0; c < 3 && c < static_cast<int>(wname.size()); c++) {
        int ch = static_cast<int>(wname[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool VRCXWorldsScreen::OnSelect(int index) {
    if (index < static_cast<int>(worlds_.size())) {
        Logger::Info("WORLDS: selected #" + std::to_string(index) + " " + worlds_[index].world_name);
        pda_.SetSelectedWorld(&worlds_[index]);
        pda_.SetPendingNavigate("VRCX_WORLD_DETAIL");
    }
    return true;
}

std::string VRCXWorldsScreen::FormatDuration(int64_t seconds) {
    if (seconds <= 0) return "  <1m";
    if (seconds < 3600) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%3ldm", static_cast<long>(seconds / 60));
        return buf;
    }
    int hrs = static_cast<int>(seconds / 3600);
    int mins = static_cast<int>((seconds % 3600) / 60);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%d:%02d", hrs, mins);
    return buf;
}

} // namespace YipOS

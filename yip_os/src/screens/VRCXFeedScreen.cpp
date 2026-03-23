#include "VRCXFeedScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

VRCXFeedScreen::VRCXFeedScreen(PDAController& pda) : ListScreen(pda) {
    name = "FEED";
    macro_index = 11;
    LoadData();
}

void VRCXFeedScreen::LoadData() {
    auto* vrcx = pda_.GetVRCXData();
    if (vrcx && vrcx->IsOpen()) {
        feed_ = vrcx->GetFeed(200);
    }
}

void VRCXFeedScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(feed_.size())) return;

    auto& f = feed_[idx];

    char indicator = (f.type == "Online") ? '+' : '-';
    std::string time_str = FormatTime(f.created_at);
    int time_len = static_cast<int>(time_str.size());
    int content_width = COLS - 2;
    int name_max = content_width - time_len - 1;
    std::string line;
    line += indicator;
    std::string dname = f.display_name;
    if (static_cast<int>(dname.size()) > name_max - 1) {
        dname = dname.substr(0, name_max - 1);
    }
    line += dname;

    for (int c = 0; c < static_cast<int>(line.size()); c++) {
        int ch = static_cast<int>(line[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }

    int time_col = COLS - 1 - time_len;
    d.WriteText(time_col, row, time_str);
}

void VRCXFeedScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(feed_.size())) return;

    auto& f = feed_[idx];
    char chars[3];
    chars[0] = (f.type == "Online") ? '+' : '-';
    chars[1] = f.display_name.size() > 0 ? f.display_name[0] : ' ';
    chars[2] = f.display_name.size() > 1 ? f.display_name[1] : ' ';

    for (int c = 0; c < 3; c++) {
        int ch = static_cast<int>(chars[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool VRCXFeedScreen::OnSelect(int index) {
    if (index < static_cast<int>(feed_.size())) {
        pda_.SetSelectedFeed(&feed_[index]);
        pda_.SetPendingNavigate("VRCX_FEED_DETAIL");
    }
    return true;
}

std::string VRCXFeedScreen::FormatTime(const std::string& created_at) {
    if (created_at.size() >= 16) {
        return created_at.substr(11, 5);
    }
    return "     ";
}

} // namespace YipOS

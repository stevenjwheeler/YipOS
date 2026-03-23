#include "AVTRChangeScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

AVTRChangeScreen::AVTRChangeScreen(PDAController& pda) : ListScreen(pda) {
    name = "CHANGE";
    macro_index = 17;
    LoadData();
}

void AVTRChangeScreen::LoadData() {
    auto* data = pda_.GetAvatarData();
    if (data) {
        avatars_ = data->GetAvatars();
    }
}

void AVTRChangeScreen::RenderEmpty() {
    display_.WriteText(2, 2, "No avatars found");
    display_.WriteText(2, 3, "Set VRC OSC path");
    display_.WriteText(2, 4, "in Config tab");
}

void AVTRChangeScreen::Render() {
    RenderFrame("CHANGE");

    if (avatars_.empty()) {
        RenderEmpty();
    } else {
        RenderRows();
    }

    RenderStatusBar();
}

void AVTRChangeScreen::RenderDynamic() {
    if (!avatars_.empty()) {
        RenderRows();
        RenderPageIndicators();
    }
    RenderClock();
    RenderCursor();
}

void AVTRChangeScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(avatars_.size())) return;

    int content_width = COLS - 2;
    std::string dname = avatars_[idx].name;
    if (static_cast<int>(dname.size()) > content_width) {
        dname = dname.substr(0, content_width);
    }

    for (int c = 0; c < static_cast<int>(dname.size()); c++) {
        int ch = static_cast<int>(dname[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
}

void AVTRChangeScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(avatars_.size())) return;

    std::string dname = avatars_[idx].name;
    for (int c = 0; c < SEL_WIDTH && c < static_cast<int>(dname.size()); c++) {
        int ch = static_cast<int>(dname[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool AVTRChangeScreen::OnSelect(int index) {
    if (index < static_cast<int>(avatars_.size())) {
        pda_.SetSelectedAvatar(&avatars_[index]);
        pda_.SetPendingNavigate("AVTR_DETAIL");
    }
    return true;
}

} // namespace YipOS

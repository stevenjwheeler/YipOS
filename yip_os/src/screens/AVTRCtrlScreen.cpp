#include "AVTRCtrlScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/VRCAvatarData.hpp"
#include "net/OSCManager.hpp"
#include "core/Logger.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

static std::string StripVFPrefix(const std::string& name) {
    if (name.size() >= 4 && name[0] == 'V' && name[1] == 'F') {
        size_t i = 2;
        while (i < name.size() && name[i] >= '0' && name[i] <= '9') i++;
        if (i > 2 && i < name.size()) {
            if (name[i] == '/') {
                return name.substr(i + 1);
            }
            if (name[i] == '_') {
                size_t slash = name.find('/', i + 1);
                if (slash != std::string::npos) {
                    return name.substr(slash + 1);
                }
                return name.substr(i + 1);
            }
        }
    }
    return name;
}

AVTRCtrlScreen::AVTRCtrlScreen(PDAController& pda) : ListScreen(pda) {
    name = "CTRL";
    macro_index = 19;
    LoadData();
}

void AVTRCtrlScreen::LoadData() {
    auto* data = pda_.GetAvatarData();
    if (!data) return;

    if (data->GetCurrentAvatarId().empty() && !data->GetAvatars().empty()) {
        data->SetCurrentAvatarId(data->GetAvatars().front().id);
    }

    auto& current_id = data->GetCurrentAvatarId();
    if (current_id.empty()) return;

    auto params = data->GetToggleParams(current_id);
    toggles_.clear();
    toggles_.reserve(params.size());
    for (auto* p : params) {
        toggles_.push_back({p, false});
    }
}

void AVTRCtrlScreen::RenderEmpty() {
    auto* data = pda_.GetAvatarData();
    if (!data || data->GetCurrentAvatarId().empty()) {
        display_.WriteText(2, 2, "No avatar selected");
        display_.WriteText(2, 3, "Use CHANGE first");
    } else {
        display_.WriteText(2, 2, "No toggles found");
    }
}

void AVTRCtrlScreen::Render() {
    RenderFrame("CTRL");

    if (toggles_.empty()) {
        RenderEmpty();
    } else {
        RenderRows();
    }

    RenderStatusBar();
}

void AVTRCtrlScreen::RenderDynamic() {
    if (!toggles_.empty()) {
        RenderRows();
        RenderPageIndicators();
    }
    RenderClock();
    RenderCursor();
}

void AVTRCtrlScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(toggles_.size())) return;

    auto& t = toggles_[idx];

    char indicator = t.on ? '+' : '-';
    int content_width = COLS - 2;
    std::string pname = StripVFPrefix(t.param->name);
    if (static_cast<int>(pname.size()) > content_width - 1) {
        pname = pname.substr(0, content_width - 1);
    }

    std::string line;
    line += indicator;
    line += pname;

    for (int c = 0; c < static_cast<int>(line.size()); c++) {
        int ch = static_cast<int>(line[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
}

void AVTRCtrlScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(toggles_.size())) return;

    auto& t = toggles_[idx];
    char chars[3];
    chars[0] = t.on ? '+' : '-';
    std::string pname = StripVFPrefix(t.param->name);
    chars[1] = pname.size() > 0 ? pname[0] : ' ';
    chars[2] = pname.size() > 1 ? pname[1] : ' ';

    for (int c = 0; c < 3; c++) {
        int ch = static_cast<int>(chars[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool AVTRCtrlScreen::OnSelect(int index) {
    if (index >= static_cast<int>(toggles_.size())) return true;

    auto& t = toggles_[index];
    t.on = !t.on;

    auto* osc = pda_.GetOSCManager();
    if (osc && t.param) {
        osc->SendBool(t.param->input_address, t.on);
        osc->SendInt(t.param->input_address, t.on ? 1 : 0);
        Logger::Info("Toggle " + t.param->name + " -> " +
                     t.param->input_address + " = " +
                     (t.on ? "ON" : "OFF"));
    }

    // Re-render just this row
    display_.CancelBuffered();
    display_.BeginBuffered();
    RenderRow(cursor_, true);
    RenderPageIndicators();
    return true;
}

} // namespace YipOS

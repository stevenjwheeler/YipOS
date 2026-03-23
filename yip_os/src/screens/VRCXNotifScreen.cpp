#include "VRCXNotifScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>

namespace YipOS {

using namespace Glyphs;

VRCXNotifScreen::VRCXNotifScreen(PDAController& pda) : ListScreen(pda) {
    name = "NOTIF";
    macro_index = 20;
    last_seen_at_ = pda_.GetConfig().GetState("notif.last_seen");
    LoadData();
    pda_.MarkNotifsSeen();
}

void VRCXNotifScreen::LoadData() {
    auto* vrcx = pda_.GetVRCXData();
    if (vrcx && vrcx->IsOpen()) {
        notifs_ = vrcx->GetNotifications(300);
    }
}

void VRCXNotifScreen::RenderDynamic() {
    // Check confirmation / done timeout
    if (clear_confirming_ || clear_done_) {
        double now = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        double timeout = clear_done_ ? DONE_TIMEOUT : CONFIRM_TIMEOUT;
        if ((now - clear_confirm_time_) >= timeout) {
            clear_confirming_ = false;
            clear_done_ = false;
            refresh_interval = 0;
        }
    }

    RenderRows();
    RenderClearButton();
    RenderPageIndicators();
    RenderClock();
    RenderCursor();
}

void VRCXNotifScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(notifs_.size())) return;

    auto& n = notifs_[idx];

    bool unseen = !last_seen_at_.empty() ? (n.created_at > last_seen_at_) : true;
    char indicator = unseen ? '*' : ' ';

    std::string type_str = FormatType(n.type);
    std::string time_str = FormatTime(n.created_at);
    int time_len = static_cast<int>(time_str.size());

    int usable_width = COLS - LEFT_COL - 1;
    int sender_max = usable_width - time_len - 6;
    std::string sender = n.sender;
    if (static_cast<int>(sender.size()) > sender_max) {
        sender = sender.substr(0, sender_max);
    }

    std::string line;
    line += indicator;
    line += type_str;
    line += ' ';
    line += sender;

    for (int c = 0; c < static_cast<int>(line.size()); c++) {
        int ch = static_cast<int>(line[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(LEFT_COL + c, row, ch);
    }

    int time_col = COLS - 1 - time_len;
    d.WriteText(time_col, row, time_str);
}

void VRCXNotifScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(notifs_.size())) return;

    auto& n = notifs_[idx];
    bool unseen = !last_seen_at_.empty() ? (n.created_at > last_seen_at_) : true;
    char chars[3];
    chars[0] = unseen ? '*' : ' ';
    std::string type_str = FormatType(n.type);
    chars[1] = type_str.size() > 0 ? type_str[0] : ' ';
    chars[2] = type_str.size() > 1 ? type_str[1] : ' ';

    for (int c = 0; c < 3; c++) {
        int ch = static_cast<int>(chars[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(LEFT_COL + c, row, ch);
    }
}

std::string VRCXNotifScreen::FormatTime(const std::string& created_at) {
    if (created_at.size() >= 16) {
        return created_at.substr(11, 5);
    }
    return "     ";
}

std::string VRCXNotifScreen::FormatType(const std::string& type) {
    if (type == "invite") return "INVT";
    if (type == "friendRequest") return "FREQ";
    if (type == "requestInvite") return "RINV";
    if (type == "requestInviteResponse") return "RRES";
    if (type == "votetokick") return "KICK";
    if (type == "informational") return "INFO";
    std::string out = type.substr(0, 4);
    for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

void VRCXNotifScreen::RenderClearButton() {
    const char* line1 = clear_done_ ? " DONE  " : (clear_confirming_ ? " SUR?  " : "CLR ALL");
    const char* line2 = clear_done_ ? "       " : (clear_confirming_ ? "       " : "NOTIFS ");
    int l1 = static_cast<int>(std::strlen(line1));
    int l2 = static_cast<int>(std::strlen(line2));
    display_.WriteText(BTN_CENTER - l1 / 2, 5, line1, true);
    display_.WriteText(BTN_CENTER - l2 / 2, 6, line2, true);
}

bool VRCXNotifScreen::OnInput(const std::string& key) {
    // Handle CLR ALL button (touch 33) before delegating to base
    if (key == "33") {
        double now = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        display_.CancelBuffered();

        if (clear_confirming_ && (now - clear_confirm_time_) < CONFIRM_TIMEOUT) {
            pda_.ClearAllNotifs();
            if (!notifs_.empty()) {
                last_seen_at_ = notifs_[0].created_at;
            }
            clear_confirming_ = false;
            clear_done_ = true;
            clear_confirm_time_ = now;
            refresh_interval = 1.0f;
            Logger::Info("Notifications cleared");
        } else {
            clear_confirming_ = true;
            clear_done_ = false;
            clear_confirm_time_ = now;
            refresh_interval = 1.0f;
        }

        display_.BeginBuffered();
        RenderClearButton();
        RenderRows();
        return true;
    }

    return ListScreen::OnInput(key);
}

} // namespace YipOS

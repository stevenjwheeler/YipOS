#include "DMMessageScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/DMClient.hpp"
#include <chrono>
#include <ctime>

namespace YipOS {

using namespace Glyphs;

DMMessageScreen::DMMessageScreen(PDAController& pda) : Screen(pda) {
    name = "DM_MSG";
    macro_index = 42;

    std::string session_id = pda_.GetSelectedDMSession();
    auto* session = pda_.GetDMClient().GetSession(session_id);
    if (session) {
        peer_name_ = session->peer_name;
        int idx = pda_.GetSelectedDMMessage();
        if (idx >= 0 && idx < static_cast<int>(session->messages.size())) {
            msg_ = &session->messages[idx];
        }
    }
}

void DMMessageScreen::Render() {
    Screen::Render();
}

void DMMessageScreen::RenderDynamic() {
    RenderContent();
    RenderClock();
    RenderCursor();
}

void DMMessageScreen::RenderContent() {
    auto& d = display_;

    if (!msg_) {
        d.WriteText(2, 3, "No message selected");
        return;
    }

    int max_w = COLS - 2; // 38

    // Row 1: "YOU" or peer name (left) + timestamp (right)
    std::string sender = msg_->is_mine ? "YOU" : peer_name_;
    std::string ts = FormatTimestamp(msg_->date);

    int name_max = max_w - static_cast<int>(ts.size()) - 1;
    if (static_cast<int>(sender.size()) > name_max)
        sender = sender.substr(0, name_max);

    d.WriteText(1, 1, sender);
    d.WriteText(COLS - 1 - static_cast<int>(ts.size()), 1, ts);

    // Rows 3-6: full message text, word-wrapped (4 rows x 38 cols)
    std::string text = msg_->text;
    for (auto& ch : text) {
        if (ch == '\n' || ch == '\r') ch = ' ';
    }

    int text_row = 3;
    int chars_on_line = 0;

    for (size_t i = 0; i < text.size() && text_row <= 6; i++) {
        if (chars_on_line >= max_w) {
            text_row++;
            chars_on_line = 0;
            if (text_row > 6) break;
        }
        d.WriteChar(1 + chars_on_line, text_row, static_cast<int>(text[i]));
        chars_on_line++;
    }
}

std::string DMMessageScreen::FormatTimestamp(int64_t date) {
    std::time_t t = static_cast<std::time_t>(date);
    std::tm* lt = std::localtime(&t);
    if (!lt) return "???";

    char buf[16];
    std::strftime(buf, sizeof(buf), "%b %d %H:%M", lt);
    return buf;
}

} // namespace YipOS

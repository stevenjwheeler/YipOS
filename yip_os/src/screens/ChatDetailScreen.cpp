#include "ChatDetailScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/ChatClient.hpp"
#include "core/Logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace YipOS {

using namespace Glyphs;

ChatDetailScreen::ChatDetailScreen(PDAController& pda) : Screen(pda) {
    name = "CHAT_DTL";
    macro_index = 26;
    msg_ = pda.GetSelectedChat();
}

void ChatDetailScreen::Render() {
    RenderFrame("CHAT DTL");
    RenderContent();
    RenderStatusBar();
}

void ChatDetailScreen::RenderDynamic() {
    RenderContent();
    RenderClock();
    RenderCursor();
}

void ChatDetailScreen::RenderContent() {
    auto& d = display_;

    if (!msg_) {
        d.WriteText(2, 3, "No message selected");
        return;
    }

    int max_w = COLS - 2; // 38

    // Row 1: username (left) + timestamp (right)
    std::string username = msg_->from;
    std::string ts = FormatTimestamp(msg_->date);

    int name_max = max_w - static_cast<int>(ts.size()) - 1;
    if (static_cast<int>(username.size()) > name_max)
        username = username.substr(0, name_max);

    d.WriteText(1, 1, username);
    d.WriteText(COLS - 1 - static_cast<int>(ts.size()), 1, ts);

    // Rows 3-6: full message text, word-wrapped (4 rows x 38 cols = 152 chars)
    std::string text = msg_->text;
    // Replace newlines with spaces for wrapping
    for (auto& ch : text) {
        if (ch == '\n' || ch == '\r') ch = ' ';
    }

    int text_row = 3;
    int text_col = 1;
    int chars_on_line = 0;

    for (size_t i = 0; i < text.size() && text_row <= 6; i++) {
        if (chars_on_line >= max_w) {
            text_row++;
            text_col = 1;
            chars_on_line = 0;
            if (text_row > 6) break;
        }
        d.WriteChar(text_col + chars_on_line, text_row, static_cast<int>(text[i]));
        chars_on_line++;
    }
}

std::string ChatDetailScreen::FormatTimestamp(int64_t date) {
    std::time_t t = static_cast<std::time_t>(date);
    std::tm* lt = std::localtime(&t);
    if (!lt) return "???";

    char buf[16];
    std::strftime(buf, sizeof(buf), "%b %d %H:%M", lt);
    return buf;
}

bool ChatDetailScreen::OnInput(const std::string& key) {
    return false;
}

} // namespace YipOS

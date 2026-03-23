#include "ChatScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <cstdio>
#include <chrono>

namespace YipOS {

using namespace Glyphs;

ChatScreen::ChatScreen(PDAController& pda) : ListScreen(pda) {
    name = "CHAT";

    std::string consent = pda_.GetConfig().GetState("chat.consent");
    if (!consent.empty()) {
        mode_ = FEED;
        macro_index = FEED_MACRO;

        messages_ = pda_.GetChatClient().GetMessages();
        if (!messages_.empty()) {
            pda_.MarkChatSeen();
            messages_ = pda_.GetChatClient().GetMessages();
        }
    } else {
        mode_ = CONSENT;
        macro_index = CONSENT_MACRO;
        consent_shown_at_ = std::chrono::steady_clock::now();
    }
}

void ChatScreen::RenderEmpty() {
    display_.WriteText(2, 3, "No messages yet");
}

void ChatScreen::Render() {
    if (mode_ == CONSENT) {
        RenderFrame("CHAT");
        RenderConsent();
        RenderStatusBar();
    } else {
        ListScreen::Render();
    }
}

void ChatScreen::RenderDynamic() {
    if (mode_ == CONSENT) {
        RenderConsent();
        RenderClock();
        RenderCursor();
    } else {
        ListScreen::RenderDynamic();
    }
}

void ChatScreen::RenderConsent() {
    // Static text is in the macro; nothing extra to render dynamically
}

void ChatScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(messages_.size())) return;

    auto& msg = messages_[idx];

    char indicator = msg.seen ? ' ' : '*';

    std::string sender = msg.from;
    if (static_cast<int>(sender.size()) > 6)
        sender = sender.substr(0, 6);

    std::string text = msg.text;
    for (auto& ch : text) {
        if (ch == '\n' || ch == '\r') ch = ' ';
    }
    if (static_cast<int>(text.size()) > 25) {
        text = text.substr(0, 22) + "...";
    }

    std::string time_str = FormatRelativeTime(msg.date);
    int content_width = COLS - 2;

    std::string line;
    line += indicator;
    line += sender;
    line += ": ";
    line += text;

    int time_col = content_width - static_cast<int>(time_str.size());
    while (static_cast<int>(line.size()) < time_col)
        line += ' ';
    if (static_cast<int>(line.size()) > time_col)
        line = line.substr(0, time_col);

    for (int c = 0; c < static_cast<int>(line.size()); c++) {
        int ch = static_cast<int>(line[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }

    d.WriteText(1 + time_col, row, time_str);
}

void ChatScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(messages_.size())) return;

    auto& msg = messages_[idx];
    char chars[3];
    chars[0] = msg.seen ? ' ' : '*';
    chars[1] = msg.from.size() > 0 ? msg.from[0] : ' ';
    chars[2] = msg.from.size() > 1 ? msg.from[1] : ' ';

    for (int c = 0; c < 3; c++) {
        int ch = static_cast<int>(chars[c]);
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool ChatScreen::OnSelect(int index) {
    if (index < static_cast<int>(messages_.size())) {
        pda_.SetSelectedChat(&messages_[index]);
        pda_.SetPendingNavigate("CHAT_DTL");
    }
    return true;
}

std::string ChatScreen::FormatRelativeTime(int64_t date) {
    auto now = std::chrono::system_clock::now();
    int64_t now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    int64_t diff = now_ts - date;

    if (diff < 0) diff = 0;

    char buf[8];
    if (diff < 60) {
        return "<1m";
    } else if (diff < 3600) {
        std::snprintf(buf, sizeof(buf), "%ldm", static_cast<long>(diff / 60));
    } else if (diff < 86400) {
        std::snprintf(buf, sizeof(buf), "%ldh", static_cast<long>(diff / 3600));
    } else if (diff < 604800) {
        std::snprintf(buf, sizeof(buf), "%ldd", static_cast<long>(diff / 86400));
    } else {
        std::snprintf(buf, sizeof(buf), "%ldw", static_cast<long>(diff / 604800));
    }
    return buf;
}

bool ChatScreen::OnInput(const std::string& key) {
    if (mode_ == CONSENT) {
        if (key == "53") {
            double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - consent_shown_at_).count();
            if (elapsed < CONSENT_DELAY_SEC) {
                Logger::Debug("Chat consent ignored (too soon: " +
                              std::to_string(elapsed) + "s)");
                return true;
            }
            pda_.GetConfig().SetState("chat.consent", "1");
            Logger::Info("Chat consent given");

            mode_ = FEED;
            macro_index = FEED_MACRO;

            pda_.GetChatClient().FetchMessages();
            messages_ = pda_.GetChatClient().GetMessages();
            if (!messages_.empty()) {
                pda_.MarkChatSeen();
                messages_ = pda_.GetChatClient().GetMessages();
            }

            pda_.StartRender(this);
            return true;
        }
        return false;
    }

    // In FEED mode, delegate to ListScreen base
    return ListScreen::OnInput(key);
}

} // namespace YipOS

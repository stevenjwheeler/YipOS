#pragma once

#include "ListScreen.hpp"
#include "net/ChatClient.hpp"
#include <vector>
#include <chrono>

namespace YipOS {

class ChatScreen : public ListScreen {
public:
    ChatScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

protected:
    int ItemCount() const override { return static_cast<int>(messages_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    void RenderEmpty() override;
    bool OnSelect(int index) override;

private:
    enum Mode { CONSENT, FEED };

    void RenderConsent();
    static std::string FormatRelativeTime(int64_t date);

    Mode mode_ = CONSENT;
    std::vector<ChatMessage> messages_;
    static constexpr int CONSENT_MACRO = 24;
    static constexpr int FEED_MACRO = 25;
    static constexpr double CONSENT_DELAY_SEC = 2.0;

    std::chrono::steady_clock::time_point consent_shown_at_{};
};

} // namespace YipOS

#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

struct ChatMessage;

class ChatDetailScreen : public Screen {
public:
    ChatDetailScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

private:
    void RenderContent();
    static std::string FormatTimestamp(int64_t date);

    const ChatMessage* msg_ = nullptr;
};

} // namespace YipOS

#pragma once

#include "Screen.hpp"
#include <string>
#include <vector>

namespace YipOS {

class DMComposeScreen : public Screen {
public:
    DMComposeScreen(PDAController& pda);
    ~DMComposeScreen() override;

    void Render() override;
    void RenderDynamic() override;
    void Update() override;
    bool OnInput(const std::string& key) override;

private:
    void StartCC();
    void StopCC();
    void RedrawText();
    void UpdateButtonLabel();
    bool FilterText(const std::string& text) const;
    void WordWrap(const std::string& text, std::vector<std::string>& output);

    static constexpr int TEXT_FIRST_ROW = 2;
    static constexpr int TEXT_LAST_ROW = 5;
    static constexpr int LINE_WIDTH = 38;
    static constexpr int MAX_COMPOSE_LENGTH = 500;
    static constexpr int COMPOSE_MACRO = 41;

    std::string session_id_;
    std::string peer_name_;
    std::string compose_buffer_;
    bool started_by_screen_ = false;
    bool paused_ = false;
    bool cc_available_ = false;

    // Flash state for sent/error feedback
    double flash_until_ = 0;
    enum class FlashState { NONE, SENT, ERROR } flash_ = FlashState::NONE;

    bool needs_redraw_ = false;
};

} // namespace YipOS

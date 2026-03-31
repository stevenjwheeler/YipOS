#include "DMComposeScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "audio/AudioCapture.hpp"
#include "audio/WhisperWorker.hpp"
#include "net/DMClient.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "core/TimeUtil.hpp"

namespace YipOS {

using namespace Glyphs;

DMComposeScreen::DMComposeScreen(PDAController& pda) : Screen(pda) {
    name = "DM_COMPOSE";
    update_interval = 0.25f;

    session_id_ = pda_.GetSelectedDMSession();
    auto* session = pda_.GetDMClient().GetSession(session_id_);
    if (session) {
        peer_name_ = session->peer_name;
    }

    StartCC();

    if (cc_available_) {
        macro_index = COMPOSE_MACRO;
    } else {
        macro_index = -1;  // dynamic render for "not configured" message
    }
}

DMComposeScreen::~DMComposeScreen() {
    if (started_by_screen_) {
        StopCC();
    }
}

void DMComposeScreen::StartCC() {
    auto* whisper = pda_.GetWhisperWorker();
    auto* audio = pda_.GetAudioCapture();
    if (!whisper || !audio) return;

    // Already running? Just use it
    if (whisper->IsRunning()) {
        cc_available_ = true;
        return;
    }

    // Load saved model from NVRAM
    if (!whisper->IsModelLoaded()) {
        std::string saved_model = pda_.GetConfig().GetState("cc.model", "tiny.en");
        std::string path = WhisperWorker::DefaultModelPath(saved_model);
        if (!whisper->LoadModel(path)) {
            path = WhisperWorker::DefaultModelPath("tiny.en");
            whisper->LoadModel(path);
        }
    }

    if (!whisper->IsModelLoaded()) {
        Logger::Warning("DMCompose: No CC model available");
        cc_available_ = false;
        return;
    }

    // Restore saved step/window
    std::string saved_step = pda_.GetConfig().GetState("cc.step");
    if (!saved_step.empty()) {
        whisper->SetStepMs(std::stoi(saved_step));
    }
    std::string saved_window = pda_.GetConfig().GetState("cc.window");
    if (!saved_window.empty()) {
        whisper->SetLengthMs(std::stoi(saved_window));
    }

    // Restore saved audio device
    std::string saved_device = pda_.GetConfig().GetState("cc.device");
    if (!saved_device.empty()) {
        audio->SetDevice(saved_device);
    }

    audio->Start();
    whisper->Start(audio->GetBuffer());
    started_by_screen_ = true;
    cc_available_ = true;

    Logger::Info("DMCompose: CC started");
}

void DMComposeScreen::StopCC() {
    auto* whisper = pda_.GetWhisperWorker();
    auto* audio = pda_.GetAudioCapture();
    if (whisper && whisper->IsRunning()) whisper->Stop();
    if (audio && audio->IsRunning()) audio->Stop();
    started_by_screen_ = false;
    Logger::Info("DMCompose: CC stopped");
}

bool DMComposeScreen::FilterText(const std::string& text) const {
    if (text.empty()) return true;
    if (text.find("[BLANK_AUDIO]") != std::string::npos) return true;
    if (text.find("[SILENCE]") != std::string::npos) return true;
    if (text.find("[ Silence ]") != std::string::npos) return true;
    if (text.find("(silence)") != std::string::npos) return true;
    if (text.find("[Music]") != std::string::npos) return true;
    if (text.find("[foreign language]") != std::string::npos) return true;
    bool has_alpha = false;
    for (char c : text) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            has_alpha = true;
            break;
        }
    }
    if (!has_alpha) return true;
    return false;
}

void DMComposeScreen::WordWrap(const std::string& text,
                                std::vector<std::string>& output) {
    size_t pos = 0;
    while (pos < text.size()) {
        size_t remaining = text.size() - pos;
        if (static_cast<int>(remaining) <= LINE_WIDTH) {
            output.push_back(text.substr(pos));
            break;
        }
        size_t end = pos + LINE_WIDTH;
        size_t break_at = text.rfind(' ', end);
        if (break_at == std::string::npos || break_at <= pos) {
            break_at = end;
        }
        output.push_back(text.substr(pos, break_at - pos));
        pos = break_at;
        if (pos < text.size() && text[pos] == ' ') pos++;
    }
}

void DMComposeScreen::Render() {
    // Only called when macro_index = -1 (CC not available)
    RenderFrame("COMPOSE");
    display_.WriteGlyph(0, 1, G_LEFT_A);
    display_.WriteText(2, 2, "CC not configured");
    display_.WriteText(2, 3, "Set up in CC > CONF");
    display_.WriteText(2, 4, "to compose messages");
    RenderStatusBar();
}

void DMComposeScreen::RenderDynamic() {
    // Write peer name after "To: " in macro
    if (!peer_name_.empty()) {
        std::string label = peer_name_;
        if (static_cast<int>(label.size()) > 33)
            label = label.substr(0, 33);
        display_.WriteText(5, 1, label);
    }

    if (cc_available_) {
        display_.WriteText(1, 2, "Listening...");
    }

    RenderClock();
    RenderCursor();
}

void DMComposeScreen::RedrawText() {
    auto* whisper = pda_.GetWhisperWorker();

    // Build committed lines
    std::vector<std::string> committed_lines;
    if (!compose_buffer_.empty()) {
        WordWrap(compose_buffer_, committed_lines);
    }

    // Build tentative lines (inverted)
    std::vector<std::string> tentative_lines;
    if (!paused_ && whisper) {
        std::string tent = whisper->GetTentative();
        if (!tent.empty() && !FilterText(tent)) {
            WordWrap(tent, tentative_lines);
        }
    }

    int total_rows = TEXT_LAST_ROW - TEXT_FIRST_ROW + 1; // 4 rows
    int total_lines = static_cast<int>(committed_lines.size()) +
                      static_cast<int>(tentative_lines.size());

    // Take the last N lines that fit
    int skip = total_lines > total_rows ? total_lines - total_rows : 0;

    int row = TEXT_FIRST_ROW;
    int line_idx = 0;

    // Write committed lines
    for (int i = 0; i < static_cast<int>(committed_lines.size()); i++) {
        if (line_idx < skip) { line_idx++; continue; }
        if (row > TEXT_LAST_ROW) break;

        // Clear row first
        for (int c = 1; c <= LINE_WIDTH; c++)
            display_.WriteChar(c, row, static_cast<int>(' '));

        auto& line = committed_lines[i];
        for (int c = 0; c < static_cast<int>(line.size()) && c < LINE_WIDTH; c++) {
            display_.WriteChar(1 + c, row, static_cast<int>(line[c]));
        }
        row++;
        line_idx++;
    }

    // Write tentative lines (inverted)
    for (int i = 0; i < static_cast<int>(tentative_lines.size()); i++) {
        if (line_idx < skip) { line_idx++; continue; }
        if (row > TEXT_LAST_ROW) break;

        // Clear row first
        for (int c = 1; c <= LINE_WIDTH; c++)
            display_.WriteChar(c, row, static_cast<int>(' '));

        auto& line = tentative_lines[i];
        for (int c = 0; c < static_cast<int>(line.size()) && c < LINE_WIDTH; c++) {
            display_.WriteChar(1 + c, row,
                               static_cast<int>(line[c]) + INVERT_OFFSET);
        }
        row++;
        line_idx++;
    }

    // Clear remaining rows
    while (row <= TEXT_LAST_ROW) {
        for (int c = 1; c <= LINE_WIDTH; c++)
            display_.WriteChar(c, row, static_cast<int>(' '));
        row++;
    }
}

void DMComposeScreen::UpdateButtonLabel() {
    // Overwrite the STOP/GO label at the macro position (col 17, row 6)
    if (paused_) {
        // Write "GO" + clear remaining space from "STOP"
        for (int i = 0; i < 4; i++) {
            int ch = static_cast<int>(' ') + INVERT_OFFSET;
            display_.WriteChar(17 + i, 6, ch);
        }
        std::string label = "GO";
        for (int i = 0; i < static_cast<int>(label.size()); i++) {
            display_.WriteChar(17 + i, 6,
                               static_cast<int>(label[i]) + INVERT_OFFSET);
        }
    } else {
        std::string label = "STOP";
        for (int i = 0; i < static_cast<int>(label.size()); i++) {
            display_.WriteChar(17 + i, 6,
                               static_cast<int>(label[i]) + INVERT_OFFSET);
        }
    }
}

void DMComposeScreen::Update() {
    if (!cc_available_) return;

    // Handle flash state
    if (flash_ != FlashState::NONE) {
        double now = MonotonicNow();
        if (now >= flash_until_) {
            if (flash_ == FlashState::SENT) {
                flash_ = FlashState::NONE;
                pda_.PopScreen();
                return;
            }
            // ERROR: clear flash, stay on screen
            flash_ = FlashState::NONE;
            needs_redraw_ = true;
        }
        return;  // don't update text while flashing
    }

    auto* whisper = pda_.GetWhisperWorker();
    if (!whisper) return;

    // Pull committed text
    while (whisper->HasCommitted()) {
        std::string text = whisper->PopCommitted();
        if (FilterText(text)) continue;

        // Append with space separator
        if (!compose_buffer_.empty() && compose_buffer_.back() != ' ') {
            compose_buffer_ += ' ';
        }
        compose_buffer_ += text;

        // Trim to max length
        if (static_cast<int>(compose_buffer_.size()) > MAX_COMPOSE_LENGTH) {
            compose_buffer_ = compose_buffer_.substr(0, MAX_COMPOSE_LENGTH);
        }
        needs_redraw_ = true;
    }

    // Check tentative version
    if (!paused_) {
        int version = static_cast<int>(whisper->GetTentativeVersion());
        if (version != last_tentative_version_) {
            last_tentative_version_ = version;
            needs_redraw_ = true;
        }
    }

    if (needs_redraw_) {
        RedrawText();
        needs_redraw_ = false;
    }
}

bool DMComposeScreen::OnInput(const std::string& key) {
    if (!cc_available_) return false;

    // CLEAR — contact 13 (col 1, row 3)
    if (key == "13") {
        compose_buffer_.clear();
        last_tentative_version_ = -1;
        RedrawText();
        return true;
    }

    // STOP/GO — contact 33 (col 3, row 3)
    if (key == "33") {
        auto* audio = pda_.GetAudioCapture();
        if (!audio) return true;

        paused_ = !paused_;
        if (paused_) {
            audio->Stop();
        } else {
            audio->Start();
        }
        UpdateButtonLabel();
        needs_redraw_ = true;
        return true;
    }

    // SEND — contact 53 (col 5, row 3)
    if (key == "53") {
        if (compose_buffer_.empty()) return true;

        if (pda_.GetDMClient().SendMessage(session_id_, compose_buffer_)) {
            Logger::Info("DMCompose: sent message (" +
                         std::to_string(compose_buffer_.size()) + " chars)");
            // Refresh messages so they show up when we pop back
            pda_.GetDMClient().FetchMessages(session_id_, 0);

            // Show "Sent!" flash
            flash_ = FlashState::SENT;
            flash_until_ = MonotonicNow() + 1.5;

            // Clear text area and show confirmation
            for (int r = TEXT_FIRST_ROW; r <= TEXT_LAST_ROW; r++) {
                for (int c = 1; c <= LINE_WIDTH; c++)
                    display_.WriteChar(c, r, static_cast<int>(' '));
            }
            display_.WriteText(2, 3, "Sent!");
        } else {
            Logger::Warning("DMCompose: send failed");
            flash_ = FlashState::ERROR;
            flash_until_ = MonotonicNow() + 1.5;

            for (int r = TEXT_FIRST_ROW; r <= TEXT_LAST_ROW; r++) {
                for (int c = 1; c <= LINE_WIDTH; c++)
                    display_.WriteChar(c, r, static_cast<int>(' '));
            }
            display_.WriteText(2, 3, "Send failed!");
        }
        return true;
    }

    return false;
}

} // namespace YipOS

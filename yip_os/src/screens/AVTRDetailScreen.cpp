#include "AVTRDetailScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/VRCAvatarData.hpp"
#include "net/OSCManager.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

AVTRDetailScreen::AVTRDetailScreen(PDAController& pda) : Screen(pda) {
    name = "AVTR DTL";
    macro_index = 18;
    avatar_ = pda.GetSelectedAvatar();
    if (avatar_) {
        auto* data = pda_.GetAvatarData();
        if (data) {
            toggle_count_ = static_cast<int>(data->GetToggleParams(avatar_->id).size());
        }
    }
}

void AVTRDetailScreen::WriteInverted(int col, int row, const std::string& text) {
    for (int i = 0; i < static_cast<int>(text.size()); i++) {
        int ch = static_cast<int>(text[i]) + INVERT_OFFSET;
        display_.WriteChar(col + i, row, ch);
    }
}

void AVTRDetailScreen::FlashButton(int col, int row, const std::string& text) {
    for (int i = 0; i < static_cast<int>(text.size()); i++)
        display_.WriteChar(col + i, row, static_cast<int>(text[i]));
    for (int i = 0; i < static_cast<int>(text.size()); i++)
        display_.WriteChar(col + i, row, static_cast<int>(text[i]) + INVERT_OFFSET);
}

void AVTRDetailScreen::RenderContent() {
    if (!avatar_) {
        display_.WriteText(2, 2, "No avatar selected");
        return;
    }

    // Row 1: Avatar name (truncated)
    std::string dname = avatar_->name;
    if (static_cast<int>(dname.size()) > COLS - 4) {
        dname = dname.substr(0, COLS - 4);
    }
    display_.WriteText(1, 1, dname);

    // Row 2: Avatar ID (truncated)
    std::string id_str = avatar_->id;
    if (static_cast<int>(id_str.size()) > COLS - 4) {
        id_str = id_str.substr(0, COLS - 4);
    }
    display_.WriteText(1, 2, id_str);

    // Row 3: Parameter counts
    char stats[40];
    std::snprintf(stats, sizeof(stats), "Params: %d  Toggles: %d",
                  static_cast<int>(avatar_->parameters.size()), toggle_count_);
    display_.WriteText(1, 3, stats);

    // Row 5: Current avatar indicator
    auto* data = pda_.GetAvatarData();
    if (data && data->GetCurrentAvatarId() == avatar_->id) {
        display_.WriteText(1, 5, "(current)");
    }

    // Row 6: APPLY button (touch 53)
    WriteInverted(COLS - 1 - 5, 6, "APPLY");
}

void AVTRDetailScreen::Render() {
    RenderFrame("AVTR DTL");
    RenderContent();
    RenderStatusBar();
}

void AVTRDetailScreen::RenderDynamic() {
    RenderContent();
    RenderClock();
    RenderCursor();
}

bool AVTRDetailScreen::OnInput(const std::string& key) {
    if (key == "53" && avatar_) {
        // Apply avatar change
        display_.CancelBuffered();
        display_.BeginBuffered();
        FlashButton(COLS - 1 - 5, 6, "APPLY");

        auto* osc = pda_.GetOSCManager();
        if (osc) {
            osc->SendString("/avatar/change", avatar_->id);
            Logger::Info("Avatar change: " + avatar_->name + " (" + avatar_->id + ")");
        }

        auto* data = pda_.GetAvatarData();
        if (data) {
            data->SetCurrentAvatarId(avatar_->id);
        }
        pda_.GetConfig().SetState("avtr.current", avatar_->id);

        display_.WriteText(1, 5, "(applied)");
        return true;
    }

    return false;
}

} // namespace YipOS

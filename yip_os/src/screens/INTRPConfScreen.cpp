#include "INTRPConfScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "audio/WhisperWorker.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"

namespace YipOS {

using namespace Glyphs;

INTRPConfScreen::INTRPConfScreen(PDAController& pda) : Screen(pda) {
    name = "INTRP CONF";
    macro_index = 36;
}

std::string INTRPConfScreen::GetLangLabel(const std::string& code) const {
    if (code == "en") return "ENGLISH";
    if (code == "es") return "ESPANOL";
    if (code == "fr") return "FRANCAIS";
    if (code == "de") return "DEUTSCH";
    if (code == "it") return "ITALIANO";
    if (code == "ja") return "JAPANESE";
    if (code == "pt") return "PORTUGUES";
    return code;
}

void INTRPConfScreen::Render() {
    RenderFrame("INTRP CONF");
    RenderContent();
    RenderStatusBar();
}

void INTRPConfScreen::RenderDynamic() {
    RenderContent();
    RenderClock();
    RenderCursor();
}

void INTRPConfScreen::RenderContent() {
    auto& d = display_;
    auto& config = pda_.GetConfig();

    std::string my_lang = config.GetState("intrp.my_lang", "en");
    std::string their_lang = config.GetState("intrp.their_lang", "es");

    // Row 1: "I SPEAK" + inverted language label (right-aligned)
    std::string my_label = GetLangLabel(my_lang);
    int my_col = COLS - 1 - static_cast<int>(my_label.size()) - 1;
    for (int i = 0; i < static_cast<int>(my_label.size()); i++) {
        d.WriteChar(my_col + i, 1, static_cast<int>(my_label[i]) + INVERT_OFFSET);
    }

    // Row 3: "THEY SPEAK" + inverted language label (right-aligned)
    std::string their_label = GetLangLabel(their_lang);
    int their_col = COLS - 1 - static_cast<int>(their_label.size()) - 1;
    for (int i = 0; i < static_cast<int>(their_label.size()); i++) {
        d.WriteChar(their_col + i, 3, static_cast<int>(their_label[i]) + INVERT_OFFSET);
    }

    // Row 5: Model + status
    auto* whisper = pda_.GetWhisperWorker();
    std::string model_str = "Model: ";
    if (whisper && whisper->IsModelLoaded())
        model_str += whisper->GetModelName();
    else
        model_str += "(none)";
    d.WriteText(1, 5, model_str);
}

bool INTRPConfScreen::OnInput(const std::string& key) {
    if (key.size() != 2) return false;
    int tx = key[0] - '1'; // touch column 0-4
    int ty = key[1] - '1'; // touch row 0-2

    // Only the right-side inverted language labels are buttons (col 5 = tx 4)
    if (tx != 4) return false;

    // Touch 51 (screen row 1): "I SPEAK" language
    if (ty == 0) {
        pda_.GetConfig().SetState("intrp.editing", "my");
        pda_.SetPendingNavigate("INTRP_LANG");
        return true;
    }

    // Touch 52 (screen row 3): "THEY SPEAK" language
    if (ty == 1) {
        pda_.GetConfig().SetState("intrp.editing", "their");
        pda_.SetPendingNavigate("INTRP_LANG");
        return true;
    }

    return false;
}

} // namespace YipOS

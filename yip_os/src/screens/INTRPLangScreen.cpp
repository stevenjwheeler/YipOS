#include "INTRPLangScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"

namespace YipOS {

using namespace Glyphs;

const std::vector<INTRPLangScreen::LangEntry> INTRPLangScreen::LANGUAGES = {
    {"en", "English"},
    {"es", "Espanol"},
    {"fr", "Francais"},
    {"de", "Deutsch"},
    {"it", "Italiano"},
    {"ja", "Japanese"},
    {"pt", "Portugues"},
};

INTRPLangScreen::INTRPLangScreen(PDAController& pda) : ListScreen(pda) {
    name = "INTRP LANG";
    macro_index = 43;

    editing_field_ = pda_.GetConfig().GetState("intrp.editing", "their");

    // Pre-select current language
    std::string nvram_key = (editing_field_ == "my") ? "intrp.my_lang" : "intrp.their_lang";
    std::string current = pda_.GetConfig().GetState(nvram_key, "en");
    for (int i = 0; i < static_cast<int>(LANGUAGES.size()); i++) {
        if (LANGUAGES[i].code == current) {
            page_ = i / ROWS_PER_PAGE;
            cursor_ = i % ROWS_PER_PAGE;
            break;
        }
    }
}

int INTRPLangScreen::ItemCount() const {
    return static_cast<int>(LANGUAGES.size());
}

void INTRPLangScreen::RenderRow(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + (i % ROWS_PER_PAGE);
    if (idx >= static_cast<int>(LANGUAGES.size())) return;

    const std::string& label = LANGUAGES[idx].label;
    int len = static_cast<int>(label.size());
    for (int c = 0; c < len; c++) {
        int ch = static_cast<int>(label[c]);
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
}

void INTRPLangScreen::WriteSelectionMark(int i, bool selected) {
    auto& d = display_;
    int idx = GlobalIndex(i);
    int row = 1 + (i % ROWS_PER_PAGE);
    if (idx >= static_cast<int>(LANGUAGES.size())) return;

    const std::string& label = LANGUAGES[idx].label;
    for (int c = 0; c < SEL_WIDTH; c++) {
        int ch = (c < static_cast<int>(label.size())) ? static_cast<int>(label[c]) : ' ';
        if (selected) ch += INVERT_OFFSET;
        d.WriteChar(1 + c, row, ch);
    }
}

bool INTRPLangScreen::OnSelect(int index) {
    if (index < 0 || index >= static_cast<int>(LANGUAGES.size())) return false;

    const auto& lang = LANGUAGES[index];
    std::string nvram_key = (editing_field_ == "my") ? "intrp.my_lang" : "intrp.their_lang";
    pda_.GetConfig().SetState(nvram_key, lang.code);
    Logger::Info("INTRP: Set " + editing_field_ + " language to " + lang.code);

    pda_.SetPendingNavigate("__POP__");
    return true;
}

} // namespace YipOS

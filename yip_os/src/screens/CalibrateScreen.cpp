#include "CalibrateScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Glyphs.hpp"
#include "core/Logger.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

CalibrateScreen::CalibrateScreen(PDAController& pda) : Screen(pda) {
    name = "CLBR";
    macro_index = -1; // no macro — pure text
}

void CalibrateScreen::RenderGlyphPage() {
    auto& d = display_;
    int bank = page_;

    // Row 0: header
    if (bank == 0) {
        d.WriteText(1, 0, "BANK 0  Standard ROM  0-255");
    } else {
        d.WriteText(1, 0, "BANK 1  Extended ROM  0-238");
    }

    // Page indicator (inverted, touchable)
    char prev_ch = '<';
    char next_ch = '>';
    d.WriteChar(0, 0, static_cast<int>(prev_ch) + INVERT_OFFSET);
    d.WriteChar(COLS - 1, 0, static_cast<int>(next_ch) + INVERT_OFFSET);

    // Rows 1-7: glyphs, 40 per row
    int max_glyph = (bank == 0) ? 255 : 238;
    int idx = 0;
    for (int row = 1; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if (idx <= max_glyph) {
                d.WriteChar(col, row, idx, bank);
            } else {
                d.WriteChar(col, row, 32, 0); // space (bank 0)
            }
            idx++;
        }
    }
}

void CalibrateScreen::Render() {
    RenderGlyphPage();
}

void CalibrateScreen::RenderContent() {}

bool CalibrateScreen::OnInput(const std::string& key) {
    Logger::Info("CLBR touch: " + key);

    // Any touch toggles between bank 0 and bank 1
    page_ = (page_ + 1) % 2;
    pda_.StartRender(this);
    return true;
}

} // namespace YipOS

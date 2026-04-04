#include "CalibrateScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Glyphs.hpp"
#include "core/Logger.hpp"
#include "img/QRGen.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

CalibrateScreen::CalibrateScreen(PDAController& pda) : Screen(pda) {
    name = "CLBR";
    macro_index = -1; // no macro — pure text
    skip_clock = false;
    update_interval = 0.1f;
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

void CalibrateScreen::RenderQRPage() {
    saved_write_delay_ = display_.GetWriteDelay();

    QRGen qr;
    if (!qr.Encode(QR_TEST_PAYLOAD)) {
        Logger::Warning("CLBR: QR encode failed");
        return;
    }

    display_.ClearScreen();
    display_.SetMacroMode();
    display_.StampMacro(QR_TEMPLATE_MACRO);

    display_.SetBitmapMode();
    display_.SetWriteDelay(0.07f);  // SLOW
    display_.BeginBuffered();

    auto& matrix = qr.GetMatrix();
    constexpr int OFF = QRGen::OFFSET;
    constexpr int SZ = QRGen::SIZE;

    // 1px dark margin for scanner forgiveness
    for (int i = OFF - 1; i <= OFF + SZ; i++) {
        display_.WriteChar(i, OFF - 1, 0);
        display_.WriteChar(i, OFF + SZ, 0);
        display_.WriteChar(OFF - 1, i, 0);
        display_.WriteChar(OFF + SZ, i, 0);
    }
    for (int r = 0; r < SZ; r++) {
        for (int c = 0; c < SZ; c++) {
            display_.WriteChar(c + OFF, r + OFF, matrix[r][c] ? 0 : 255);
        }
    }

    qr_rendering_ = true;
    Logger::Info("CLBR: QR test render started (payload=" + std::string(QR_TEST_PAYLOAD) + ")");
}

void CalibrateScreen::Render() {
    if (page_ == 2) {
        RenderQRPage();
    } else {
        RenderGlyphPage();
    }
}

void CalibrateScreen::RenderContent() {}

void CalibrateScreen::Update() {
    if (qr_rendering_ && !display_.IsBuffered()) {
        qr_rendering_ = false;
        display_.SetWriteDelay(saved_write_delay_);
        display_.SetTextMode();
        display_.WriteText(2, 7, "QRTEST 123456 - SCAN ME");
        Logger::Info("CLBR: QR render complete");
    }
}

bool CalibrateScreen::OnInput(const std::string& key) {
    Logger::Info("CLBR touch: " + key);

    // On QR page: any touch cancels the render and cycles to page 0
    if (page_ == 2) {
        if (qr_rendering_) {
            display_.CancelBuffered();
            display_.SetWriteDelay(saved_write_delay_);
            display_.SetTextMode();
            qr_rendering_ = false;
        }
    }

    // Cycle through 3 pages
    page_ = (page_ + 1) % 3;
    pda_.StartRender(this);
    return true;
}

} // namespace YipOS

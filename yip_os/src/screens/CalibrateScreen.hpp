#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

class CalibrateScreen : public Screen {
public:
    CalibrateScreen(PDAController& pda);

    void Render() override;
    void RenderContent() override;
    void Update() override;
    bool OnInput(const std::string& key) override;

private:
    int page_ = 0; // 0 = bank 0, 1 = bank 1, 2 = QR test
    void RenderGlyphPage();
    void RenderQRPage();

    bool qr_rendering_ = false;
    float saved_write_delay_ = 0.07f;
    static constexpr int QR_TEMPLATE_MACRO = 37;
    static constexpr const char* QR_TEST_PAYLOAD = "123456";
};

} // namespace YipOS

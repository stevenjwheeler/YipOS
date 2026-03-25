#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

class CalibrateScreen : public Screen {
public:
    CalibrateScreen(PDAController& pda);

    void Render() override;
    void RenderContent() override;
    bool OnInput(const std::string& key) override;

private:
    int page_ = 0; // 0 = bank 0 (standard), 1 = bank 1 (extended)
    void RenderGlyphPage();
};

} // namespace YipOS

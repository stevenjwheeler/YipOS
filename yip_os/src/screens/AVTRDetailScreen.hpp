#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

struct VRCAvatarEntry;

class AVTRDetailScreen : public Screen {
public:
    AVTRDetailScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

private:
    void RenderContent();
    void WriteInverted(int col, int row, const std::string& text);
    void FlashButton(int col, int row, const std::string& text);

    const VRCAvatarEntry* avatar_ = nullptr;
    int toggle_count_ = 0;
};

} // namespace YipOS

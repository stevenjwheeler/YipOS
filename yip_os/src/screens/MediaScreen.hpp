#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

class MediaScreen : public Screen {
public:
    MediaScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    void Update() override;
    bool OnInput(const std::string& key) override;

private:
    void RenderMediaInfo();
    void WriteTitle();
    void WriteArtist();
    void WriteStatus();
    void WriteControls();

    std::string title_;
    std::string artist_;
    int status_ = 3; // MediaInfo::Unknown
    int scroll_offset_ = 0;
    bool initialized_ = false;
};

} // namespace YipOS

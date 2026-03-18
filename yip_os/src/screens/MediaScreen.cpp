#include "MediaScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "media/MediaController.hpp"
#include "core/Glyphs.hpp"
#include "core/Logger.hpp"
#include <algorithm>

namespace YipOS {

using namespace Glyphs;

static constexpr int CONTENT_WIDTH = COLS - 2; // 38 usable cols inside frame
static constexpr int TITLE_MAX = CONTENT_WIDTH - 2; // 36, room for note glyph + space

MediaScreen::MediaScreen(PDAController& pda) : Screen(pda) {
    name = "MEDIA";
    macro_index = 29;
    update_interval = 2.0f;
}

void MediaScreen::Render() {
    RenderFrame("MEDIA");
    display_.WriteGlyph(0, 1, G_LEFT_A);
    RenderMediaInfo();
    WriteControls();
    RenderStatusBar();
}

void MediaScreen::RenderDynamic() {
    display_.WriteGlyph(0, 1, G_LEFT_A);
    RenderMediaInfo();
    WriteControls();
    RenderClock();
    RenderCursor();
}

void MediaScreen::RenderMediaInfo() {
    auto* mc = pda_.GetMediaController();
    if (!mc) {
        display_.WriteText(2, 3, "No media available");
        return;
    }

    if (!initialized_) {
        auto info = mc->GetCurrentMedia();
        title_ = info.title;
        artist_ = info.artist;
        status_ = static_cast<int>(info.status);
        initialized_ = true;
    }

    WriteTitle();
    WriteArtist();
    WriteStatus();
}

void MediaScreen::WriteTitle() {
    // Row 1: note glyph + title
    display_.WriteGlyph(1, 1, G_NOTE);

    std::string display_title = title_.empty() ? "---" : title_;

    if (static_cast<int>(display_title.size()) <= TITLE_MAX) {
        // Fits — write padded
        std::string padded = display_title;
        padded.resize(TITLE_MAX, ' ');
        display_.WriteText(3, 1, padded);
    } else {
        // Scrolling: show a window into "title   title   ..."
        std::string scrolling = display_title + "   " + display_title;
        int offset = scroll_offset_ % (static_cast<int>(display_title.size()) + 3);
        std::string window = scrolling.substr(offset, TITLE_MAX);
        display_.WriteText(3, 1, window);
    }
}

void MediaScreen::WriteArtist() {
    // Row 2: artist
    std::string display_artist = artist_.empty() ? "---" : artist_;
    if (static_cast<int>(display_artist.size()) > CONTENT_WIDTH) {
        display_artist = display_artist.substr(0, CONTENT_WIDTH);
    }
    display_artist.resize(CONTENT_WIDTH, ' ');
    display_.WriteText(1, 2, display_artist);
}

void MediaScreen::WriteStatus() {
    // Row 6: status text
    const char* status_text = "Unknown";
    switch (status_) {
        case 0: status_text = "Stopped"; break;
        case 1: status_text = "Playing"; break;
        case 2: status_text = "Paused"; break;
    }

    std::string status_str = status_text;
    status_str.resize(CONTENT_WIDTH, ' ');
    display_.WriteText(1, 6, status_str);
}

void MediaScreen::WriteControls() {
    // Row 4: PREV  PLAY/PAUS  NEXT (inverted = touchable)
    // Centered on touch columns: btn_cols [4, 20, 36] mapped to tile centers
    // Touch row 2 (ty=1) maps to ZONE_ROWS[1] = row 4

    // PREV at col ~2
    display_.WriteText(2, 4, "PREV", true);

    // PLAY or PAUS at center
    const char* play_label = (status_ == 1) ? "PAUS" : "PLAY";
    display_.WriteText(18, 4, play_label, true);

    // NEXT at right
    display_.WriteText(34, 4, "NEXT", true);
}

void MediaScreen::Update() {
    auto* mc = pda_.GetMediaController();
    if (!mc) return;

    auto info = mc->GetCurrentMedia();

    bool changed = false;

    if (info.title != title_) {
        title_ = info.title;
        scroll_offset_ = 0;
        changed = true;
    } else if (static_cast<int>(title_.size()) > TITLE_MAX) {
        scroll_offset_++;
        changed = true;
    }

    if (info.artist != artist_) {
        artist_ = info.artist;
        changed = true;
    }

    int new_status = static_cast<int>(info.status);
    if (new_status != status_) {
        status_ = new_status;
        changed = true;
    }

    if (changed) {
        display_.CancelBuffered();
        display_.BeginBuffered();
        WriteTitle();
        WriteArtist();
        WriteStatus();
        WriteControls();
    }
}

bool MediaScreen::OnInput(const std::string& key) {
    auto* mc = pda_.GetMediaController();
    if (!mc) return false;

    // Touch contacts: format "{X}{Y}" where X=1-5 (col), Y=1-3 (row)
    // Row 2 contacts align with control buttons on display row 4
    // X=1,2 → PREV, X=3 → PLAY/PAUSE, X=4,5 → NEXT

    if (key == "12" || key == "22") {
        mc->Previous();
        Logger::Info("MEDIA: Previous");
        return true;
    }
    if (key == "32") {
        mc->TogglePlayPause();
        Logger::Info("MEDIA: TogglePlayPause");
        return true;
    }
    if (key == "42" || key == "52") {
        mc->Next();
        Logger::Info("MEDIA: Next");
        return true;
    }

    // Also support joystick/TR for play/pause
    if (key == "TR" || key == "Joystick") {
        mc->TogglePlayPause();
        Logger::Info("MEDIA: TogglePlayPause (button)");
        return true;
    }

    return false;
}

} // namespace YipOS

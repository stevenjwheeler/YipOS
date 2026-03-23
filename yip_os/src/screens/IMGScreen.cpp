#include "IMGScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Logger.hpp"
#include "core/Glyphs.hpp"
#include <algorithm>
#include <filesystem>
#include <cstdio>

namespace YipOS {

using namespace Glyphs;
namespace fs = std::filesystem;

static constexpr int BMP_COLS = VQEncoder::IMG_COLS;
static constexpr int BMP_ROWS = VQEncoder::IMG_ROWS;

IMGScreen::IMGScreen(PDAController& pda) : ListScreen(pda) {
    name = "IMG";
    macro_index = 27;
    update_interval = 1.0f;

    std::string assets = pda_.GetAssetsPath();
    if (assets.empty()) assets = "assets";

    fs::path img_dir = fs::path(assets) / "images";
    if (fs::exists(img_dir) && fs::is_directory(img_dir)) {
        images_dir_ = img_dir.string();
    } else {
        images_dir_ = assets;
    }

    fs::path cb = fs::path(assets) / "vq_codebook.npy";
    if (!fs::exists(cb)) {
        for (const auto& p : {"vq_codebook.npy", "../vq_codebook.npy",
                              "../../vq_codebook.npy"}) {
            if (fs::exists(p)) { cb = p; break; }
        }
    }
    if (!encoder_.LoadCodebook(cb.string())) {
        Logger::Warning("IMG: VQ codebook not found at " + cb.string());
    }

    ScanImages();
}

void IMGScreen::ScanImages() {
    image_files_.clear();
    if (!fs::exists(images_dir_) || !fs::is_directory(images_dir_)) {
        Logger::Warning("IMG: images directory not found: " + images_dir_);
        return;
    }

    for (const auto& entry : fs::directory_iterator(images_dir_)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
            image_files_.push_back(entry.path().filename().string());
        }
    }
    std::sort(image_files_.begin(), image_files_.end());
    Logger::Info("IMG: found " + std::to_string(image_files_.size()) +
                 " images in " + images_dir_);
}

void IMGScreen::RenderEmpty() {
    display_.WriteText(2, 3, "No images found");
    display_.WriteText(2, 4, images_dir_.substr(0, COLS - 4));
}

void IMGScreen::Render() {
    if (mode_ == DISPLAY) {
        WriteBitmap();
        return;
    }
    RenderFrame("IMG");
    if (!image_files_.empty()) {
        display_.WriteGlyph(COLS - 1, 1, G_RIGHT_A);
    }
    RenderRows();
    RenderPageIndicators();
    RenderStatusBar();
}

void IMGScreen::RenderDynamic() {
    if (mode_ == DISPLAY) {
        WriteBitmap();
        return;
    }
    if (!image_files_.empty()) {
        display_.WriteGlyph(COLS - 1, 1, G_RIGHT_A);
    }
    RenderRows();
    RenderPageIndicators();
    RenderClock();
    RenderCursor();
}

void IMGScreen::RenderRow(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(image_files_.size())) return;

    const auto& fname = image_files_[idx];
    int content_width = COLS - 2;
    std::string display_name = fname;
    auto dot = display_name.rfind('.');
    if (dot != std::string::npos) display_name = display_name.substr(0, dot);
    if (static_cast<int>(display_name.size()) > content_width) {
        display_name = display_name.substr(0, content_width);
    }

    for (int c = 0; c < static_cast<int>(display_name.size()); c++) {
        int ch = static_cast<int>(display_name[c]);
        if (ch < 32 || ch > 126) ch = 32;
        if (selected && c < SEL_WIDTH) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

void IMGScreen::WriteSelectionMark(int i, bool selected) {
    int idx = GlobalIndex(i);
    int row = 1 + i;
    if (idx >= static_cast<int>(image_files_.size())) return;

    std::string display_name = image_files_[idx];
    auto dot = display_name.rfind('.');
    if (dot != std::string::npos) display_name = display_name.substr(0, dot);

    for (int c = 0; c < 3 && c < static_cast<int>(display_name.size()); c++) {
        int ch = static_cast<int>(display_name[c]);
        if (ch < 32 || ch > 126) ch = 32;
        if (selected) ch += INVERT_OFFSET;
        display_.WriteChar(1 + c, row, ch);
    }
}

bool IMGScreen::OnSelect(int index) {
    if (index < static_cast<int>(image_files_.size())) {
        std::string full_path = (fs::path(images_dir_) / image_files_[index]).string();
        EnterDisplayMode(full_path);
    }
    return true;
}

void IMGScreen::EnterDisplayMode(const std::string& image_path) {
    if (!encoder_.IsLoaded()) {
        Logger::Warning("IMG: codebook not loaded, cannot display");
        return;
    }

    Logger::Info("IMG: encoding " + image_path + "...");
    if (!encoder_.EncodeImage(image_path, vq_indices_)) {
        Logger::Error("IMG: failed to encode " + image_path);
        return;
    }

    current_image_ = image_path;
    mode_ = DISPLAY;
    skip_clock = true;
    handle_back = true;
    refresh_interval = -1.0f;
    update_interval = 0.1f;

    saved_write_delay_ = display_.GetWriteDelay();

    display_.CancelBuffered();
    display_.ClearScreen();
    display_.SetBitmapMode();

    pass_ = 0;
    pass_done_ = false;
    StartPass();

    Logger::Info("IMG: displaying " + image_path);
}

void IMGScreen::StartPass() {
    float delay = (pass_ < PASS_COUNT) ? PASS_DELAYS[pass_] : REFRESH_DELAY;
    display_.SetWriteDelay(delay);
    display_.BeginBuffered();
    WriteBitmap();
    pass_active_ = true;

    int n = display_.BufferedRemaining();
    Logger::Info("IMG: pass " + std::to_string(pass_ + 1) +
                 "/" + std::to_string(PASS_COUNT) +
                 " (delay=" + std::to_string(static_cast<int>(delay * 1000)) +
                 "ms, " + std::to_string(n) + " writes)");
}

void IMGScreen::ExitDisplayMode() {
    mode_ = LIST;
    skip_clock = false;
    handle_back = false;
    refresh_interval = 0;
    update_interval = 1.0f;
    pass_active_ = false;
    pass_done_ = false;

    display_.SetWriteDelay(saved_write_delay_);

    pda_.StartRender(this);
    Logger::Info("IMG: back to list");
}

void IMGScreen::WriteBitmap() {
    for (int r = 0; r < BMP_ROWS; r++) {
        for (int c = 0; c < BMP_COLS; c++) {
            display_.WriteChar(c, r, vq_indices_[r][c]);
        }
    }
}

void IMGScreen::Update() {
    std::string drop = pda_.ConsumeDroppedImagePath();
    if (!drop.empty()) {
        Logger::Info("IMG: Update() consumed drop, cancelling current render (" +
                     std::to_string(display_.BufferedRemaining()) + " writes pending)");
        DropImage(drop);
        return;
    }

    if (mode_ == DISPLAY && pass_active_ && !display_.IsBuffered()) {
        pass_active_ = false;
        pass_++;
        if (pass_ < PASS_COUNT) {
            StartPass();
        } else if (!pass_done_) {
            StartPass();
            pass_done_ = true;
        }
    }
}

void IMGScreen::DropImage(const std::string& path) {
    Logger::Info("IMG: dropped image " + path);

    fs::path src(path);
    fs::path dst = fs::path(images_dir_) / src.filename();
    if (src != dst && fs::exists(src)) {
        try {
            fs::create_directories(images_dir_);
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
            Logger::Info("IMG: copied " + src.filename().string() + " to images directory");
            ScanImages();
        } catch (const fs::filesystem_error& e) {
            Logger::Warning("IMG: failed to copy image: " + std::string(e.what()));
        }
    }

    EnterDisplayMode(path);
}

bool IMGScreen::OnInput(const std::string& key) {
    if (mode_ == DISPLAY) {
        if (key == "TL" || key == "Joystick" || key == "TR") {
            ExitDisplayMode();
            return true;
        }
        return true;
    }

    return ListScreen::OnInput(key);
}

} // namespace YipOS

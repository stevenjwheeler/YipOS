#pragma once

#include "ListScreen.hpp"
#include "img/VQEncoder.hpp"
#include <string>
#include <vector>

namespace YipOS {

class IMGScreen : public ListScreen {
public:
    IMGScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    void Update() override;
    bool OnInput(const std::string& key) override;

    // External drop: set a path to immediately encode and display
    void DropImage(const std::string& path);

protected:
    int ItemCount() const override { return static_cast<int>(image_files_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    void RenderEmpty() override;
    bool OnSelect(int index) override;

private:
    enum SubMode { LIST, DISPLAY };

    void ScanImages();
    void EnterDisplayMode(const std::string& image_path);
    void ExitDisplayMode();
    void StartPass();
    void WriteBitmap();

    SubMode mode_ = LIST;
    std::string images_dir_;

    // List state
    std::vector<std::string> image_files_;

    // Display state
    VQEncoder encoder_;
    VQEncoder::IndexGrid vq_indices_{};
    std::string current_image_;
    float saved_write_delay_ = 0.07f;

    // Progressive pass system
    int pass_ = 0;
    bool pass_active_ = false;
    bool pass_done_ = false;
    static constexpr int PASS_COUNT = 3;
    static constexpr float PASS_DELAYS[PASS_COUNT] = {0.01f, 0.04f, 0.07f};
    static constexpr float REFRESH_DELAY = 0.07f;

    std::string pending_drop_;
};

} // namespace YipOS

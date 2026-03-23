#pragma once

#include "ListScreen.hpp"
#include <string>
#include <vector>

namespace YipOS {

struct VRCAvatarParam;

class AVTRCtrlScreen : public ListScreen {
public:
    AVTRCtrlScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;

protected:
    int ItemCount() const override { return static_cast<int>(toggles_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    void RenderEmpty() override;
    bool OnSelect(int index) override;

private:
    void LoadData();

    struct ToggleState {
        const VRCAvatarParam* param = nullptr;
        bool on = false;
    };

    std::vector<ToggleState> toggles_;
};

} // namespace YipOS

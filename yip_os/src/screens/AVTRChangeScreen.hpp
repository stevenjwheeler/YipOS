#pragma once

#include "ListScreen.hpp"
#include "net/VRCAvatarData.hpp"
#include <vector>

namespace YipOS {

class AVTRChangeScreen : public ListScreen {
public:
    AVTRChangeScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;

protected:
    int ItemCount() const override { return static_cast<int>(avatars_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    void RenderEmpty() override;
    bool OnSelect(int index) override;

private:
    void LoadData();

    std::vector<VRCAvatarEntry> avatars_;
};

} // namespace YipOS

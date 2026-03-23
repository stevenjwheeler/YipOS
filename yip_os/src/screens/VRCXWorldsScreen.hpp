#pragma once

#include "ListScreen.hpp"
#include "net/VRCXData.hpp"
#include <vector>

namespace YipOS {

class VRCXWorldsScreen : public ListScreen {
public:
    VRCXWorldsScreen(PDAController& pda);

protected:
    int ItemCount() const override { return static_cast<int>(worlds_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    bool OnSelect(int index) override;

private:
    void LoadData();
    static std::string FormatDuration(int64_t seconds);

    std::vector<VRCXWorldEntry> worlds_;
};

} // namespace YipOS

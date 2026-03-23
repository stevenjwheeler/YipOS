#pragma once

#include "ListScreen.hpp"
#include "net/VRCXData.hpp"
#include <vector>

namespace YipOS {

class VRCXFeedScreen : public ListScreen {
public:
    VRCXFeedScreen(PDAController& pda);

protected:
    int ItemCount() const override { return static_cast<int>(feed_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    bool OnSelect(int index) override;

private:
    void LoadData();
    static std::string FormatTime(const std::string& created_at);

    std::vector<VRCXFeedEntry> feed_;
};

} // namespace YipOS

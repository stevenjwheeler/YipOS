#pragma once

#include "ListScreen.hpp"
#include <vector>
#include <string>

namespace YipOS {

class StonkListScreen : public ListScreen {
public:
    StonkListScreen(PDAController& pda);

protected:
    int ItemCount() const override { return static_cast<int>(symbols_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    bool OnSelect(int index) override;

private:
    std::vector<std::string> symbols_;
};

} // namespace YipOS

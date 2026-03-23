#pragma once

#include "ListScreen.hpp"

namespace YipOS {

class BFIParamScreen : public ListScreen {
public:
    BFIParamScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

protected:
    int ItemCount() const override;
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;
    bool OnSelect(int index) override;

private:
    int active_idx_ = 0;  // currently selected param (shown with "+")
    int max_name_len_ = 0;
};

} // namespace YipOS

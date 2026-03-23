#pragma once

#include "ListScreen.hpp"
#include "net/VRCXData.hpp"
#include <vector>

namespace YipOS {

class VRCXNotifScreen : public ListScreen {
public:
    VRCXNotifScreen(PDAController& pda);

    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

protected:
    int ItemCount() const override { return static_cast<int>(notifs_.size()); }
    void RenderRow(int i, bool selected) override;
    void WriteSelectionMark(int i, bool selected) override;

private:
    void LoadData();
    static std::string FormatTime(const std::string& created_at);
    static std::string FormatType(const std::string& type);
    void RenderClearButton();

    std::vector<VRCXNotifEntry> notifs_;
    std::string last_seen_at_;

    // CLR ALL double-tap confirmation
    bool clear_confirming_ = false;
    bool clear_done_ = false;
    double clear_confirm_time_ = 0.0;
    static constexpr double CONFIRM_TIMEOUT = 3.0;
    static constexpr double DONE_TIMEOUT = 2.0;
    static constexpr int BTN_CENTER = 20;
    static constexpr int LEFT_COL = 2;
};

} // namespace YipOS

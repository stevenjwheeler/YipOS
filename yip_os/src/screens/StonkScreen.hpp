#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

class StonkScreen : public Screen {
public:
    StonkScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;
    void Update() override;

private:
    void SyncFromConfig();
    void RenderGraph();
    void RenderInfoLine();
    void CycleTimeWindow();
    std::string FormatScale(float price) const;
    std::string FormatPrice(float price) const;
    std::string FormatChange(float current, float prev_close) const;

    static constexpr int GRAPH_LEFT = 5;
    static constexpr int GRAPH_RIGHT = 37;
    static constexpr int GRAPH_TOP = 1;
    static constexpr int GRAPH_BOTTOM = 5;
    static constexpr int GRAPH_WIDTH = GRAPH_RIGHT - GRAPH_LEFT + 1;   // 33
    static constexpr int GRAPH_HEIGHT = GRAPH_BOTTOM - GRAPH_TOP + 1;  // 5
    static constexpr int GRAPH_LEVELS = GRAPH_HEIGHT * 2;              // 10

    std::string selected_symbol_;
    std::string time_window_;
    int64_t last_data_fetch_ = 0;

    static constexpr const char* TIME_WINDOWS[] = {"1DY", "1WK", "1MO", "6MO", "1YR", "5YR"};
    static constexpr int TIME_WINDOW_COUNT = 6;
};

} // namespace YipOS

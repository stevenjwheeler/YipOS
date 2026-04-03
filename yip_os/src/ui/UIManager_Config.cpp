#include "UIManager.hpp"
#include "app/PDAController.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"

#include <imgui.h>
#include <cstdlib>

namespace YipOS {

void UIManager::RenderNVRAMTab(PDAController& pda, Config& config) {
    ImGui::Text("NVRAM (Persistent State)");
    ImGui::TextDisabled("Key-value store saved to config.ini [state] section.");
    ImGui::TextDisabled("Used by screens to remember preferences across restarts.");

    ImGui::Separator();

    ImGui::Text("%d key%s stored", static_cast<int>(config.state.size()),
                config.state.size() == 1 ? "" : "s");

    if (!config.state.empty()) {
        ImGui::Separator();
        for (auto& [key, val] : config.state) {
            ImGui::BulletText("%s = %s", key.c_str(), val.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("Clear All NVRAM")) {
            config.ClearState();
        }
        ImGui::TextDisabled("This will reset all saved preferences (disk, network, CC settings, etc.).");
    }
}

void UIManager::RenderLogTab() {
    auto& path = Logger::GetLogPath();
    if (!path.empty()) {
        ImGui::TextDisabled("Log file: %s", path.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Open")) {
#ifdef _WIN32
            // Open Explorer to the folder (file may be locked by the logger)
            std::string dir = path.substr(0, path.find_last_of("\\/"));
            std::string cmd = "explorer \"" + dir + "\"";
#elif __APPLE__
            std::string cmd = "open -R \"" + path + "\"";
#else
            std::string dir = path.substr(0, path.find_last_of('/'));
            std::string cmd = "xdg-open \"" + dir + "\" &";
#endif
            std::system(cmd.c_str());
        }
    }

    ImGui::Checkbox("Auto-scroll", &log_auto_scroll_);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_lines_.clear();
    }

    ImGui::Separator();
    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        for (const auto& line : log_lines_) {
            ImGui::TextUnformatted(line.c_str());
        }
    }
    if (log_auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

} // namespace YipOS

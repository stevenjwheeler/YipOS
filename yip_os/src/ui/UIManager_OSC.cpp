#include "UIManager.hpp"
#include "app/PDAController.hpp"
#include "core/Config.hpp"
#include "net/OSCManager.hpp"
#include "net/OSCQueryServer.hpp"
#include "screens/BFIData.hpp"

#include <imgui.h>
#include <cstdio>

namespace YipOS {

void UIManager::RenderOSCTab(PDAController& pda, Config& config, OSCManager& osc) {
    ImGui::Text("OSC Configuration");
    ImGui::TextDisabled("YipOS uses OSC Query for automatic VRChat service discovery.");

    ImGui::Separator();

    // --- OSC Query toggle ---
    {
        bool query_enabled = config.osc_query_enabled;
        if (ImGui::Checkbox("Enable OSC Query", &query_enabled)) {
            config.osc_query_enabled = query_enabled;
            if (!query_enabled) {
                // Immediately stop OSCQuery from overriding the send target
                manual_osc_override_ = true;
                osc.SetSendTarget(config.osc_ip, config.osc_send_port);
            } else {
                manual_osc_override_ = false;
            }
            if (!config_path_.empty()) config.SaveToFile(config_path_);
        }
        if (config.osc_query_enabled != (osc_query_ != nullptr)) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(restart to apply)");
        }
    }

    // --- OSC Query Status ---
    if (osc_query_) {
        ImGui::Text("OSC Query");
        if (osc_query_->IsVRChatConnected()) {
            auto port = osc_query_->GetVRChatOSCPort();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f),
                "VRChat discovered (OSC port %d)", port ? *port : 0);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                "Searching for VRChat...");
        }
        ImGui::TextDisabled("HTTP port: %d  |  OSC listen port: %d",
            osc_query_->GetHTTPPort(), osc_query_->GetOSCPort());
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
            "Note: Close Unity if open -- its OSC listener can block VRChat discovery.");
    } else if (config.osc_query_enabled) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
            "OSC Query failed to start — using static ports");
    } else {
        ImGui::TextDisabled("OSC Query is disabled — using static ports");
    }

    ImGui::Separator();

    if (osc.IsRunning()) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "OSC Status: Running");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "OSC Status: Disconnected");
    }

    ImGui::Separator();

    // --- Static port config (collapsible fallback) ---
    if (ImGui::CollapsingHeader("Manual Port Configuration")) {
        ImGui::TextDisabled("Only needed when OSC Query is disabled or unavailable.");
        ImGui::Spacing();

        static char ip_buf[64] = {};
        if (ip_buf[0] == 0) {
            std::snprintf(ip_buf, sizeof(ip_buf), "%s", config.osc_ip.c_str());
        }

        ImGui::InputText("IP Address", ip_buf, sizeof(ip_buf));
        ImGui::InputInt("Send Port", &config.osc_send_port);
        ImGui::TextDisabled("Port to send OSC messages to (VRChat default: 9000)");
        ImGui::InputInt("Listen Port", &config.osc_listen_port);
        ImGui::TextDisabled("Port to receive OSC messages on (requires restart)");

        ImGui::Spacing();
        if (ImGui::Button("Apply & Save")) {
            config.osc_ip = ip_buf;
            osc.SetSendTarget(config.osc_ip, config.osc_send_port);
            manual_osc_override_ = true;
            if (!config_path_.empty()) config.SaveToFile(config_path_);
        }
        if (manual_osc_override_) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Manual override active (OSCQuery won't change port)");
            ImGui::SameLine();
            if (ImGui::SmallButton("Release")) {
                manual_osc_override_ = false;
            }
        }
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("Simulate OSC Input");
    ImGui::TextDisabled("Inject values as if received from external apps (heart rate, BFI, etc.).");

    // --- Heart Rate ---
    ImGui::Spacing();
    ImGui::Text("Heart Rate");
    static int sim_hr_bpm = 75;
    ImGui::SliderInt("BPM", &sim_hr_bpm, 40, 200);
    ImGui::SameLine();
    if (ImGui::Button("Send HR")) {
        pda.SetHeartRate(sim_hr_bpm);
    }
    ImGui::Checkbox("Auto (1 Hz)", &sim_hr_auto_);
    ImGui::SameLine();
    ImGui::TextDisabled("Sends current slider value every second");
    // Auto HR tick runs in TickSimulations() so it works when tab is hidden

    // HEART listens for these params (matched on final path segment)
    ImGui::Spacing();
    ImGui::TextDisabled("HEART listens on these OSC param names (final path segment):");
    ImGui::BulletText("Int 0-255 BPM: HR, Heartrate3, HeartRateInt");
    ImGui::BulletText("Float -1..1: Heartrate, HeartRateFloat, FullHRPercent");

    // Custom param input
    ImGui::Spacing();
    ImGui::Text("Custom Param");
    ImGui::TextDisabled("Avatar parameter name to also listen to (just the name, no path).");

    if (!heart_custom_param_initialized_) {
        std::string p = config.GetState("heart.custom_param");
        std::snprintf(heart_custom_param_buf_.data(), heart_custom_param_buf_.size(), "%s", p.c_str());
        heart_custom_param_initialized_ = true;
    }
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##heart_custom", heart_custom_param_buf_.data(), heart_custom_param_buf_.size());
    ImGui::SameLine();
    static int heart_type_idx = (config.GetState("heart.custom_param_type", "int") == "float") ? 1 : 0;
    const char* heart_types[] = { "Int 0-255", "Float -1..1" };
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("##heart_type", &heart_type_idx, heart_types, 2)) {
        config.SetState("heart.custom_param_type", heart_type_idx == 1 ? "float" : "int");
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply##heart")) {
        std::string name(heart_custom_param_buf_.data());
        while (!name.empty() && name.front() == ' ') name.erase(name.begin());
        while (!name.empty() && name.back() == ' ') name.pop_back();
        if (!name.empty() && name[0] == '/') name.erase(name.begin());
        config.SetState("heart.custom_param", name);
    }

    // --- BFI ---
    ImGui::Spacing();
    ImGui::Text("BFI (BrainFlowsIntoVRChat) — %d params", BFI_PARAM_COUNT);
    ImGui::Checkbox("Auto BFI (1 Hz)", &sim_bfi_auto_);
    ImGui::SameLine();
    ImGui::TextDisabled("Sends sine waves to all %d params", BFI_PARAM_COUNT);
    // Auto BFI tick runs in TickSimulations()
}

} // namespace YipOS

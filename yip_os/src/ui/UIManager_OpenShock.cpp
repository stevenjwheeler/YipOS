/**
 * OpenShockClient.cpp
 * V1.0.0
 *
 * Adds OpenShock API integration to YipOS for remote control of OpenShock
 * devices.
 *
 * By otter_oasis
 */

#include "UIManager.hpp"
#include "app/PDAController.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "net/OpenShockClient.hpp"

#include <algorithm>
#include <cstdio>
#include <imgui.h>

namespace YipOS {

void UIManager::RenderOpenShockTab(PDAController &pda, Config &config) {
  ImGui::Text("OpenShock Integration");
  ImGui::TextDisabled(
      "Drive your OpenShock devices directly from the Yip-Boi.");

  ImGui::Separator();

  auto *client = pda.GetOpenShockClient();
  if (!client) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                       "OpenShock client not initialized.");
    return;
  }

  // Initialize buffer from config state
  if (!openshock_token_initialized_) {
    std::string token = config.GetState("openshock.token");
    std::snprintf(openshock_token_buf_.data(), openshock_token_buf_.size(),
                  "%s", token.c_str());

    std::string i_step = config.GetState("openshock.intensity_step", "2.5");
    try {
      openshock_intensity_step_ = std::stof(i_step);
    } catch (...) {
    }

    std::string d_step = config.GetState("openshock.duration_step", "1000");
    try {
      openshock_duration_step_ = std::stoi(d_step);
    } catch (...) {
    }

    openshock_token_initialized_ = true;
  }

  // --- Status ---
  if (!client->HasToken()) {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                       "Status: Offline (no token)");
  } else if (client->IsTokenValid()) {
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Status: Connected");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                       "Status: Authentication Failed");
  }

  ImGui::Spacing();

  // --- Token ---
  ImGui::Text("API Token");
  ImGui::SetNextItemWidth(-1);
  ImGui::InputText("##os_token", openshock_token_buf_.data(),
                   openshock_token_buf_.size(), ImGuiInputTextFlags_Password);
  ImGui::TextDisabled(
      "Generate a token at https://next.openshock.app/settings/api-tokens");

  ImGui::Spacing();

  // --- Increments ---
  ImGui::Text("Control Increments (Yip-Boi Screen)");
  ImGui::SetNextItemWidth(150);
  ImGui::InputFloat("Intensity Step (%)", &openshock_intensity_step_, 0.5f,
                    5.0f, "%.1f");
  ImGui::SetNextItemWidth(150);
  ImGui::InputInt("Duration Step (ms)", &openshock_duration_step_, 100, 1000);

  ImGui::Spacing();

  // --- Actions ---
  if (ImGui::Button("Apply & Save Settings")) {
    std::string token(openshock_token_buf_.data());
    // Robust trim (all whitespace)
    token.erase(token.begin(),
                std::find_if(token.begin(), token.end(), [](unsigned char ch) {
                  return !std::isspace(ch);
                }));
    token.erase(std::find_if(token.rbegin(), token.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                token.end());

    config.SetState("openshock.token", token);
    config.SetState("openshock.intensity_step",
                    std::to_string(openshock_intensity_step_));
    config.SetState("openshock.duration_step",
                    std::to_string(openshock_duration_step_));

    client->SetToken(token);
    client->FetchShockers();

    if (!config_path_.empty())
      config.SaveToFile(config_path_);
    Logger::Info("OpenShock settings updated.");
  }

  ImGui::Separator();
  ImGui::Text("Tools");

  if (ImGui::Button("Refresh Shocker List")) {
    client->FetchShockers();
  }

  ImGui::SameLine();

  if (ImGui::Button("Test Vibration")) {
    const auto &shockers = client->GetShockers();
    if (!shockers.empty()) {
      // Vibrate first device for 1s at 20%
      client->SendControl(shockers[0].id, "Vibrate", 20.0f, 1000);
      Logger::Info("OpenShock: Sent test vibration to " + shockers[0].name);
    } else {
      Logger::Warning("OpenShock: No shockers available to test.");
    }
  }
  ImGui::TextDisabled(
      "Sends a 1.0s vibrate at 20%% intensity to the first available device.");
}

} // namespace YipOS

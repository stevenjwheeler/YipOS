/**
 * OpenShockClient.cpp
 * V1.0.0
 *
 * Adds OpenShock API integration to YipOS for remote control of OpenShock
 * devices.
 *
 * By otter_oasis
 */

#include "OpenShockScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Config.hpp"
#include "core/Glyphs.hpp"
#include "core/Logger.hpp"
#include "core/TimeUtil.hpp"
#include "net/OpenShockClient.hpp"
#include <algorithm>
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

OpenShockScreen::OpenShockScreen(PDAController &pda) : Screen(pda) {
  name = "SHOCK";
  macro_index = 48; // OPENSHOCK macro
}

void OpenShockScreen::Render() {
  // The OPENSHOCK title and layout is baked into the macro texture
  RenderContent();
  RenderStatusBar();
}

void OpenShockScreen::RenderContent() {
  RenderShockerSelection();
  RenderModeSelection();

  // Draw feedback if flashing, otherwise restore the default label area
  if (show_success_flash_) {
    display_.WriteText(25, 6, " [  SENT!  ] ", true);
  } else {
    display_.WriteText(25, 6, " [ EXECUTE ] ", true);
  }

  // Intensity Value
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%3.0f%%", intensity_);
  display_.WriteText(8, 4, buf);

  // Duration Value
  std::snprintf(buf, sizeof(buf), "%1.1fs", duration_ms_ / 1000.0f);
  display_.WriteText(28, 4, buf, false);
}

void OpenShockScreen::RenderDynamic() {
  if (show_success_flash_ && MonotonicNow() > flash_end_time_) {
    show_success_flash_ = false;
  }

  RenderContent();
  RenderClock();
  RenderCursor();
}

void OpenShockScreen::RenderShockerSelection() {
  auto *client = pda_.GetOpenShockClient();
  if (!client)
    return;

  if (!client->HasToken()) {
    display_.WriteText(8, 1, "SETUP IN APP");
    return;
  }

  const auto &items = client->GetShockers();
  std::string label = "NO DEVICES OR BAD TOKEN";
  if (!items.empty()) {
    if (selected_shocker_idx_ >= static_cast<int>(items.size())) {
      selected_shocker_idx_ = 0;
    }
    label = items[selected_shocker_idx_].name;
  }

  display_.WriteGlyph(1, 1, G_TRACKER);

  // Name centered between arrows (cols 8 to 31), padded to 24 chars to clear
  // old text
  std::string padded = label.substr(0, 24);
  if (padded.length() < 24)
    padded.append(24 - padded.length(), ' ');
  display_.WriteText(8, 1, padded);
}

void OpenShockScreen::RenderModeSelection() {
  auto *client = pda_.GetOpenShockClient();

  // Status indicator at Column 0 (to the left of ACTIVE MODE label) on Row 3
  std::string status = "- ";
  if (client) {
    if (client->HasToken()) {
      status = client->IsTokenValid() ? "  " : "! ";
    }
  }
  display_.WriteText(2, 1, status);

  std::string val =
      (!client || !client->HasToken() || client->GetShockers().empty())
          ? "UNAVAILABLE"
          : MODES[mode_idx_];

  // Add hazard markers if in SHOCK mode
  if (mode_idx_ == 0 && client && client->HasToken()) {
    val = "!!" + std::string(MODES[0]) + "!!";
  }

  // Pad with spaces to clear old text (e.g. switching from !!SHOCK!! to VIBE)
  if (val.length() < 11) {
    val += std::string(11 - val.length(), ' ');
  }

  // Positioned at Col 17 to sit directly after the new macro label position
  // with a space on Row 3
  display_.WriteText(17, 3, val);
}

bool OpenShockScreen::OnInput(const std::string &key) {
  if (key == "TL") {
    pda_.PopScreen();
    return true;
  }

  if (key.size() == 2 && key[0] >= '1' && key[0] <= '5' && key[1] >= '1' &&
      key[1] <= '3') {
    int tx = key[0] - '1';
    int ty = key[1] - '1';

    auto *client = pda_.GetOpenShockClient();
    auto &config = pda_.GetConfig();
    bool changed = false;

    if (ty == 0) {   // Top Row: Devices (Unchanged)
      if (tx == 0) { // Previous
        if (selected_shocker_idx_ > 0) {
          selected_shocker_idx_--;
          changed = true;
        }
      } else if (tx == 4) { // Next
        if (client && selected_shocker_idx_ <
                          static_cast<int>(client->GetShockers().size()) - 1) {
          selected_shocker_idx_++;
          changed = true;
        }
      }
    } else if (ty == 1) { // Middle Row: Intensity (Left) & Duration (Right)
      float i_step = 2.5f;
      int d_step = 1000;
      try {
        i_step = std::stof(config.GetState("openshock.intensity_step", "2.5"));
      } catch (...) {
      }
      try {
        d_step = std::stoi(config.GetState("openshock.duration_step", "1000"));
      } catch (...) {
      }

      if (tx == 0) { // Int Down
        intensity_ = std::max(0.0f, intensity_ - i_step);
        changed = true;
      } else if (tx == 2) { // Int Up
        intensity_ = std::min(100.0f, intensity_ + i_step);
        changed = true;
      } else if (tx == 3) { // Dur Down
        duration_ms_ = std::max(100, duration_ms_ - d_step);
        changed = true;
      } else if (tx == 4) { // Dur Up
        duration_ms_ = std::min(30000, duration_ms_ + d_step);
        changed = true;
      }
    } else if (ty == 2) { // Bottom Row: Mode Cycle (Left) & Execute (Right)
      if (tx <= 1) {      // Mode (tx 0-1)
        mode_idx_ = (mode_idx_ + 1) % 3;
        changed = true;
      } else if (tx >= 3) { // EXECUTE (tx 3-4)
        if (client && !client->GetShockers().empty()) {
          const auto &s = client->GetShockers()[selected_shocker_idx_];
          client->SendControl(s.id, MODES[mode_idx_], intensity_, duration_ms_);
          show_success_flash_ = true;
          flash_end_time_ = MonotonicNow() + 2.0;
          changed = true;
        }
      }
    }

    if (changed) {
      display_.BeginBuffered();
      RenderContent();
    }
    return true;
  }

  return false;
}

void OpenShockScreen::Update() {
  static double last_fetch = 0;
  if (MonotonicNow() - last_fetch > 60.0) {
    if (auto *client = pda_.GetOpenShockClient()) {
      client->FetchShockers();
    }
    last_fetch = MonotonicNow();
  }
}

} // namespace YipOS

#include "StayScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "net/OSCManager.hpp"
#include "core/Logger.hpp"
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

// Display labels: what the PDA shows
const char* StayScreen::DISPLAY_LABELS[PART_ROWS][PART_ACTIVE_COLS] = {
    {"LW",    "RW",    "COLLAR"},
    {"LF",    "RF",    "ALL"},
};

// Map [row][col] → SPVR device index (PDAController::SPVR_DEVICE_NAMES)
// 0=HMD, 1=ControllerLeft, 2=ControllerRight, 3=FootLeft, 4=FootRight, 5=Hip
// -1 = ALL (global lock/unlock)
const int StayScreen::SPVR_INDEX[PART_ROWS][PART_ACTIVE_COLS] = {
    {1, 2, 0},   // LW=ControllerLeft, RW=ControllerRight, COLLAR=HMD
    {3, 4, -1},  // LF=FootLeft, RF=FootRight, ALL=global
};

const std::array<int, 3> StayScreen::TILE_POSITIONS = {
    Glyphs::TILE_CENTERS[0] - 3,
    Glyphs::TILE_CENTERS[1] - 3,
    Glyphs::TILE_CENTERS[3] - 3,
};

const std::unordered_map<std::string, StayScreen::PartMapping> StayScreen::CONTACT_MAP = {
    {"12", {0, 0}}, // LW
    {"13", {1, 0}}, // LF
    {"22", {0, 1}}, // RW
    {"23", {1, 1}}, // RF
    {"42", {0, 2}}, // COLLAR
    {"43", {1, 2}}, // ALL
};

StayScreen::StayScreen(PDAController& pda) : Screen(pda) {
    name = "STAY";
    macro_index = 2;
    update_interval = 2;

    // Initialize cached status to -1 (unknown) so first Update draws everything
    for (int r = 0; r < PART_ROWS; r++)
        for (int c = 0; c < PART_ACTIVE_COLS; c++)
            last_status_[r][c] = -1;
}

const char* StayScreen::StatusLabel(int status) const {
    switch (status) {
        case 0:  return " OFF ";
        case 1:  return "FREE ";
        case 2:  return "LCKD ";
        case 3:  return "WARN ";
        case 4:  return "DSB! ";
        case 5:  return "OOB! ";
        default: return "UNKN ";
    }
}

bool StayScreen::IsLocked(int spvr_index) const {
    return pda_.GetSPVRStatus(spvr_index) >= 2;
}

void StayScreen::SendLock(int spvr_index, bool lock) {
    auto* osc = pda_.GetOSCManager();
    if (!osc) return;

    const char* device = PDAController::SPVR_DEVICE_NAMES[spvr_index];
    std::string path = std::string(Glyphs::PARAM_PREFIX) + "SPVR_" + device + "_Latch_IsPosed";
    osc->SendBool(path, lock);
    Logger::Info("SPVR: " + std::string(lock ? "Lock" : "Unlock") + " " + device);
}

void StayScreen::SendGlobalLock(bool lock) {
    auto* osc = pda_.GetOSCManager();
    if (!osc) return;

    std::string path = std::string(Glyphs::PARAM_PREFIX) +
                       (lock ? "SPVR_Global_Lock" : "SPVR_Global_Unlock");
    osc->SendBool(path, true);
    Logger::Info(std::string("SPVR: Global ") + (lock ? "Lock" : "Unlock"));
}

void StayScreen::Render() {
    RenderFrame("STAYPUTVR");

    // Row 2: separator
    display_.WriteGlyph(0, 2, G_L_TEE);
    for (int c = 1; c < COLS - 1; c++) display_.WriteGlyph(c, 2, G_HLINE);
    display_.WriteGlyph(COLS - 1, 2, G_R_TEE);

    // Body parts
    RenderPartRow(0, 3);
    RenderPartRow(1, 5);

    RenderStatusBar();
}

void StayScreen::RenderDynamic() {
    // Render all parts with current status
    for (int r = 0; r < PART_ROWS; r++)
        for (int c = 0; c < PART_ACTIVE_COLS; c++)
            RenderSinglePart(r, c);
    RenderClock();
    RenderCursor();
}

void StayScreen::RenderPartRow(int part_row, int display_row) {
    for (int i = 0; i < PART_ACTIVE_COLS; i++) {
        const char* label = DISPLAY_LABELS[part_row][i];
        if (!label) continue;

        int pos = TILE_POSITIONS[i];
        int spvr_idx = SPVR_INDEX[part_row][i];

        // For ALL, check if any device is locked
        int status;
        if (spvr_idx < 0) {
            bool any_locked = false;
            for (int d = 0; d < PDAController::SPVR_DEVICE_COUNT; d++) {
                if (pda_.GetSPVRStatus(d) >= 2) { any_locked = true; break; }
            }
            status = any_locked ? 2 : 1;
        } else {
            status = pda_.GetSPVRStatus(spvr_idx);
        }
        bool locked = (status >= 2);

        // Lock/unlock glyph + label (inverted = touchable)
        display_.WriteGlyph(pos, display_row, locked ? G_LOCK : G_UNLOCK);
        char lbl[8];
        std::snprintf(lbl, sizeof(lbl), " %-5s", label);
        display_.WriteText(pos + 1, display_row, lbl, true);

        // Status text
        const char* state_str = StatusLabel(status);
        char state_buf[9];
        std::snprintf(state_buf, sizeof(state_buf), "%-8.8s", state_str);
        display_.WriteText(pos, display_row + 1, state_buf, locked);

        last_status_[part_row][i] = status;
    }
}

void StayScreen::RenderSinglePart(int part_row, int part_col, bool flash) {
    const char* label = DISPLAY_LABELS[part_row][part_col];
    if (!label) return;

    int display_row = (part_row == 0) ? 3 : 5;
    int pos = TILE_POSITIONS[part_col];
    int spvr_idx = SPVR_INDEX[part_row][part_col];

    int status;
    if (spvr_idx < 0) {
        bool any_locked = false;
        for (int d = 0; d < PDAController::SPVR_DEVICE_COUNT; d++) {
            if (pda_.GetSPVRStatus(d) >= 2) { any_locked = true; break; }
        }
        status = any_locked ? 2 : 1;
    } else {
        status = pda_.GetSPVRStatus(spvr_idx);
    }
    bool locked = (status >= 2);

    // Flash: momentarily show label un-inverted
    if (flash) {
        char lbl[8];
        std::snprintf(lbl, sizeof(lbl), " %-5s", label);
        display_.WriteText(pos + 1, display_row, lbl, false);
    }

    // Lock glyph + inverted label
    display_.WriteGlyph(pos, display_row, locked ? G_LOCK : G_UNLOCK);
    char lbl[8];
    std::snprintf(lbl, sizeof(lbl), " %-5s", label);
    display_.WriteText(pos + 1, display_row, lbl, true);

    // Clear state row then write new status
    for (int c = 0; c < 8; c++) {
        display_.WriteChar(pos + c, display_row + 1, 32);
    }
    const char* state_str = StatusLabel(status);
    char state_buf[9];
    std::snprintf(state_buf, sizeof(state_buf), "%-8.8s", state_str);
    display_.WriteText(pos, display_row + 1, state_buf, locked);

    last_status_[part_row][part_col] = status;
}

bool StayScreen::OnInput(const std::string& key) {
    auto it = CONTACT_MAP.find(key);
    if (it == CONTACT_MAP.end()) return false;

    int ty = it->second.row;
    int tx = it->second.col;
    int spvr_idx = SPVR_INDEX[ty][tx];

    display_.CancelBuffered();

    if (spvr_idx < 0) {
        // ALL: if any locked → unlock all, else lock all
        bool any_locked = false;
        for (int d = 0; d < PDAController::SPVR_DEVICE_COUNT; d++) {
            if (pda_.GetSPVRStatus(d) >= 2) { any_locked = true; break; }
        }
        SendGlobalLock(!any_locked);
    } else {
        // Toggle individual device
        bool currently_locked = IsLocked(spvr_idx);
        SendLock(spvr_idx, !currently_locked);
    }

    // Flash the button
    RenderSinglePart(ty, tx, true);
    return true;
}

void StayScreen::Update() {
    // Check if any status changed since last render, redraw only changed parts
    display_.BeginBuffered();
    for (int r = 0; r < PART_ROWS; r++) {
        for (int c = 0; c < PART_ACTIVE_COLS; c++) {
            int spvr_idx = SPVR_INDEX[r][c];
            int status;
            if (spvr_idx < 0) {
                bool any_locked = false;
                for (int d = 0; d < PDAController::SPVR_DEVICE_COUNT; d++) {
                    if (pda_.GetSPVRStatus(d) >= 2) { any_locked = true; break; }
                }
                status = any_locked ? 2 : 1;
            } else {
                status = pda_.GetSPVRStatus(spvr_idx);
            }
            if (status != last_status_[r][c]) {
                RenderSinglePart(r, c);
            }
        }
    }
}

} // namespace YipOS

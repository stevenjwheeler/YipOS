#pragma once

namespace YipOS {

// BFI (BrainFlowsIntoVRChat) param definitions
// Each entry: { "OSC/sub/path" (after "BFI/"), "DisplayName", is_positive_only }
struct BFIParamDef {
    const char* osc_suffix;   // matched against address after "BFI/"
    const char* display_name; // shown on screen (max ~14 chars)
    bool positive_only;       // true = [0,1], false = [-1,1]
};

inline constexpr int BFI_PARAM_COUNT = 26;
inline constexpr BFIParamDef BFI_PARAMS[BFI_PARAM_COUNT] = {
    // NeuroFB — Focus
    {"NeuroFB/FocusAvg",       "FocusAvg",      false},
    {"NeuroFB/FocusPosAvg",    "FocusPosAvg",   true},
    {"NeuroFB/FocusLeft",      "FocusLeft",     false},
    {"NeuroFB/FocusPosLeft",   "FocusPosLeft",  true},
    {"NeuroFB/FocusRight",     "FocusRight",    false},
    {"NeuroFB/FocusPosRight",  "FocusPosRight", true},
    // NeuroFB — Relax
    {"NeuroFB/RelaxAvg",       "RelaxAvg",      false},
    {"NeuroFB/RelaxPosAvg",    "RelaxPosAvg",   true},
    {"NeuroFB/RelaxLeft",      "RelaxLeft",     false},
    {"NeuroFB/RelaxPosLeft",   "RelaxPosLeft",  true},
    {"NeuroFB/RelaxRight",     "RelaxRight",    false},
    {"NeuroFB/RelaxPosRight",  "RelaxPosRight", true},
    // PwrBands — Avg
    {"PwrBands/Avg/Delta",     "PB Delta",      true},
    {"PwrBands/Avg/Theta",     "PB Theta",      true},
    {"PwrBands/Avg/Alpha",     "PB Alpha",      true},
    {"PwrBands/Avg/Beta",      "PB Beta",       true},
    {"PwrBands/Avg/Gamma",     "PB Gamma",      true},
    // Biometrics
    {"Biometrics/OxygenPercent",     "O2 %",      true},
    {"Biometrics/HeartBeatsPerSecond","Heart BPS", true},
    {"Biometrics/BreathsPerSecond",  "Breath BPS",true},
    // Addons
    {"Addons/HueShift",        "HueShift",      true},
    // Info
    {"Info/BatteryLevel",      "Battery",       true},
    {"Info/SecondsSinceLastUpdate", "LastUpdate", true},
    // PwrBands — Left
    {"PwrBands/Left/Alpha",    "PB L Alpha",    true},
    // PwrBands — Right
    {"PwrBands/Right/Alpha",   "PB R Alpha",    true},
    // MLAction
    {"MLAction/ActionH",       "ML ActionH",    false},
};

} // namespace YipOS

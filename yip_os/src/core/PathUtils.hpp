#pragma once

#include <string>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace YipOS {

// Get the directory containing the running executable.
inline std::string GetExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    std::string path(buf);
    auto pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
#else
    return ".";
#endif
}

inline std::string GetConfigDir() {
#ifdef _WIN32
    // TODO: Use SHGetKnownFolderPath for FOLDERID_RoamingAppData
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::string(appdata) + "\\yip_os";
    }
    return ".\\yip_os";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        return std::string(xdg) + "/yip_os";
    }
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/yip_os";
    }
    return "./yip_os";
#endif
}

inline void EnsureDirectories(const std::string& configDir) {
    namespace fs = std::filesystem;
    fs::create_directories(configDir);
    fs::create_directories(configDir + "/logs");
}

} // namespace YipOS

#include "VRCXData.hpp"
#include "core/Logger.hpp"
#include <sqlite3.h>
#include <cstdlib>
#include <filesystem>

namespace YipOS {

VRCXData::VRCXData() = default;

VRCXData::~VRCXData() {
    Close();
}

std::string VRCXData::DefaultDBPath() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) return std::string(appdata) + "\\VRCX\\VRCX.sqlite3";
    return "";
#else
    const char* home = std::getenv("HOME");
    if (!home) return "";
    // Check XDG_CONFIG_HOME first, then fallback
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg) {
        std::string p = std::string(xdg) + "/VRCX/VRCX.sqlite3";
        if (std::filesystem::exists(p)) return p;
    }
    return std::string(home) + "/.config/VRCX/VRCX.sqlite3";
#endif
}

bool VRCXData::Open(const std::string& db_path) {
    Close();
    std::string path = db_path.empty() ? DefaultDBPath() : db_path;
    if (path.empty() || !std::filesystem::exists(path)) {
        Logger::Warning("VRCX DB not found: " + path);
        return false;
    }

    int rc = sqlite3_open_v2(path.c_str(), &db_,
                             SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (rc != SQLITE_OK) {
        Logger::Warning("VRCX DB open failed: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    sqlite3_busy_timeout(db_, 1000);

    if (!DetectUserPrefix()) {
        Logger::Warning("VRCX DB: could not detect user prefix");
    } else {
        Logger::Info("VRCX user prefix: " + user_prefix_);
    }

    return true;
}

void VRCXData::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    user_prefix_.clear();
}

bool VRCXData::DetectUserPrefix() {
    if (!db_) return false;
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_,
        "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '%_friend_log_current'",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        auto pos = name.find("_friend_log_current");
        if (pos != std::string::npos) {
            user_prefix_ = name.substr(0, pos);
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

// Helper to safely get text column
static std::string ColText(sqlite3_stmt* stmt, int col) {
    const unsigned char* t = sqlite3_column_text(stmt, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

std::vector<VRCXWorldEntry> VRCXData::GetWorlds(int limit, int offset) {
    std::vector<VRCXWorldEntry> result;
    if (!db_) return result;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_,
        "SELECT created_at, world_name, time, world_id, location, group_name "
        "FROM gamelog_location "
        "ORDER BY created_at DESC LIMIT ? OFFSET ?",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VRCXWorldEntry e;
        e.created_at = ColText(stmt, 0);
        e.world_name = ColText(stmt, 1);
        e.time_seconds = sqlite3_column_int64(stmt, 2);
        e.world_id = ColText(stmt, 3);
        e.location = ColText(stmt, 4);
        e.group_name = ColText(stmt, 5);
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<VRCXFeedEntry> VRCXData::GetFeed(int limit, int offset) {
    std::vector<VRCXFeedEntry> result;
    if (!db_ || user_prefix_.empty()) return result;

    std::string sql = "SELECT created_at, display_name, type FROM " +
                      user_prefix_ + "_feed_online_offline "
                      "ORDER BY created_at DESC LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VRCXFeedEntry e;
        e.created_at = ColText(stmt, 0);
        e.display_name = ColText(stmt, 1);
        e.type = ColText(stmt, 2);
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<VRCXStatusEntry> VRCXData::GetStatus(int limit, int offset) {
    std::vector<VRCXStatusEntry> result;
    if (!db_ || user_prefix_.empty()) return result;

    std::string sql = "SELECT created_at, display_name, status, status_description FROM " +
                      user_prefix_ + "_feed_status "
                      "ORDER BY created_at DESC LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VRCXStatusEntry e;
        e.created_at = ColText(stmt, 0);
        e.display_name = ColText(stmt, 1);
        e.status = ColText(stmt, 2);
        e.status_description = ColText(stmt, 3);
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<VRCXNotifEntry> VRCXData::GetNotifications(int limit, int offset) {
    std::vector<VRCXNotifEntry> result;
    if (!db_ || user_prefix_.empty()) return result;

    std::string sql = "SELECT created_at, type, sender_username, message FROM " +
                      user_prefix_ + "_notifications "
                      "ORDER BY created_at DESC LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VRCXNotifEntry e;
        e.created_at = ColText(stmt, 0);
        e.type = ColText(stmt, 1);
        e.sender = ColText(stmt, 2);
        e.message = ColText(stmt, 3);
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return result;
}

static int CountTable(sqlite3* db, const std::string& table) {
    sqlite3_stmt* stmt = nullptr;
    std::string sql = "SELECT count(*) FROM " + table;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return 0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

int VRCXData::GetWorldCount() {
    return db_ ? CountTable(db_, "gamelog_location") : 0;
}

int VRCXData::GetFeedCount() {
    return (db_ && !user_prefix_.empty())
        ? CountTable(db_, user_prefix_ + "_feed_online_offline") : 0;
}

int VRCXData::GetStatusCount() {
    return (db_ && !user_prefix_.empty())
        ? CountTable(db_, user_prefix_ + "_feed_status") : 0;
}

int VRCXData::GetNotifCount() {
    return (db_ && !user_prefix_.empty())
        ? CountTable(db_, user_prefix_ + "_notifications") : 0;
}

} // namespace YipOS

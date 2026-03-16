#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct sqlite3;

namespace YipOS {

struct VRCXWorldEntry {
    std::string created_at;
    std::string world_name;
    std::string world_id;
    std::string location;    // full instance string e.g. "wrld_xxx:12840~private(usr_xxx)~region(use)"
    std::string group_name;
    int64_t time_seconds;
};

struct VRCXFeedEntry {
    std::string created_at;
    std::string display_name;
    std::string type; // "Online" or "Offline"
};

struct VRCXStatusEntry {
    std::string created_at;
    std::string display_name;
    std::string status;
    std::string status_description;
};

struct VRCXNotifEntry {
    std::string created_at;
    std::string type;
    std::string sender;
    std::string message;
};

class VRCXData {
public:
    VRCXData();
    ~VRCXData();

    bool Open(const std::string& db_path = "");
    void Close();
    bool IsOpen() const { return db_ != nullptr; }

    static std::string DefaultDBPath();

    std::vector<VRCXWorldEntry> GetWorlds(int limit = 100, int offset = 0);
    std::vector<VRCXFeedEntry> GetFeed(int limit = 100, int offset = 0);
    std::vector<VRCXStatusEntry> GetStatus(int limit = 100, int offset = 0);
    std::vector<VRCXNotifEntry> GetNotifications(int limit = 100, int offset = 0);

    int GetWorldCount();
    int GetFeedCount();
    int GetStatusCount();
    int GetNotifCount();

private:
    bool DetectUserPrefix();

    sqlite3* db_ = nullptr;
    std::string user_prefix_;
};

} // namespace YipOS

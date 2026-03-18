#pragma once

#include <string>
#include <vector>
#include <cstdint>

typedef void CURL;

namespace YipOS {

struct ChatMessage {
    std::string from;       // sender display name
    std::string text;       // message text (pre-truncated by Worker to 200 chars)
    int64_t date = 0;       // unix timestamp
    bool seen = false;      // has this message been displayed on the feed screen
};

class ChatClient {
public:
    ChatClient();
    ~ChatClient();

    void SetEndpoint(const std::string& url);
    bool FetchMessages();
    const std::vector<ChatMessage>& GetMessages() const { return messages_; }
    bool HasUnseen() const;
    void MarkAllSeen(int64_t newest_date);
    int64_t GetLastSeenDate() const { return last_seen_date_; }
    void SetLastSeenDate(int64_t ts) { last_seen_date_ = ts; }

private:
    bool ParseJSON(const std::string& json);

    std::string endpoint_;
    std::vector<ChatMessage> messages_;
    int64_t last_seen_date_ = 0;
    CURL* curl_ = nullptr;
};

} // namespace YipOS

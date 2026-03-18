#include "ChatClient.hpp"
#include "core/Logger.hpp"
#include <curl/curl.h>
#include <algorithm>

namespace YipOS {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

ChatClient::ChatClient() {
    curl_ = curl_easy_init();
}

ChatClient::~ChatClient() {
    if (curl_) curl_easy_cleanup(curl_);
}

void ChatClient::SetEndpoint(const std::string& url) {
    endpoint_ = url;
}

bool ChatClient::FetchMessages() {
    if (!curl_ || endpoint_.empty()) return false;

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl_);
    if (res != CURLE_OK) {
        Logger::Warning("ChatClient fetch failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        Logger::Warning("ChatClient HTTP " + std::to_string(http_code));
        return false;
    }

    if (!ParseJSON(response)) {
        Logger::Warning("ChatClient JSON parse failed");
        return false;
    }

    // Mark seen status based on last_seen_date_
    for (auto& msg : messages_) {
        msg.seen = (msg.date <= last_seen_date_);
    }

    Logger::Debug("ChatClient fetched " + std::to_string(messages_.size()) + " messages");
    return true;
}

bool ChatClient::HasUnseen() const {
    for (auto& msg : messages_) {
        if (!msg.seen) return true;
    }
    return false;
}

void ChatClient::MarkAllSeen(int64_t newest_date) {
    last_seen_date_ = newest_date;
    for (auto& msg : messages_) {
        msg.seen = true;
    }
}

// Minimal JSON parser for the simple array format:
// [{"from":"Alice","text":"hello","date":1710700000}, ...]
// No dependencies — just enough to handle the Worker's output.
bool ChatClient::ParseJSON(const std::string& json) {
    messages_.clear();

    size_t pos = 0;
    // Skip to opening bracket
    while (pos < json.size() && json[pos] != '[') pos++;
    if (pos >= json.size()) return false;
    pos++; // skip '['

    while (pos < json.size()) {
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t' || json[pos] == ','))
            pos++;

        if (pos >= json.size() || json[pos] == ']') break;
        if (json[pos] != '{') return false;
        pos++; // skip '{'

        ChatMessage msg;
        while (pos < json.size() && json[pos] != '}') {
            // Skip whitespace and commas
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t' || json[pos] == ','))
                pos++;
            if (pos >= json.size() || json[pos] == '}') break;

            // Parse key
            if (json[pos] != '"') return false;
            pos++;
            size_t key_start = pos;
            while (pos < json.size() && json[pos] != '"') pos++;
            if (pos >= json.size()) return false;
            std::string key = json.substr(key_start, pos - key_start);
            pos++; // skip closing quote

            // Skip colon
            while (pos < json.size() && json[pos] != ':') pos++;
            if (pos >= json.size()) return false;
            pos++;

            // Skip whitespace
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

            if (json[pos] == '"') {
                // String value
                pos++;
                std::string value;
                while (pos < json.size() && json[pos] != '"') {
                    if (json[pos] == '\\' && pos + 1 < json.size()) {
                        pos++;
                        switch (json[pos]) {
                            case '"': value += '"'; break;
                            case '\\': value += '\\'; break;
                            case 'n': value += '\n'; break;
                            case 't': value += '\t'; break;
                            case '/': value += '/'; break;
                            default: value += json[pos]; break;
                        }
                    } else {
                        value += json[pos];
                    }
                    pos++;
                }
                if (pos >= json.size()) return false;
                pos++; // skip closing quote

                if (key == "from") msg.from = value;
                else if (key == "text") msg.text = value;
            } else {
                // Numeric value
                size_t num_start = pos;
                while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ' ')
                    pos++;
                std::string num_str = json.substr(num_start, pos - num_start);

                if (key == "date") {
                    try { msg.date = std::stoll(num_str); }
                    catch (...) { msg.date = 0; }
                }
            }
        }

        if (pos < json.size() && json[pos] == '}') pos++;
        messages_.push_back(std::move(msg));
    }

    // Sort newest first
    std::sort(messages_.begin(), messages_.end(),
              [](const ChatMessage& a, const ChatMessage& b) { return a.date > b.date; });

    return true;
}

} // namespace YipOS

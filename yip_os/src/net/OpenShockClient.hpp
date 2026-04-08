/**
 * OpenShockClient.hpp
 * V1.0.0
 *
 * Adds OpenShock API integration to YipOS for remote control of OpenShock
 * devices.
 *
 * By otter_oasis
 */

#pragma once

#include <string>
#include <vector>

typedef void CURL;

namespace YipOS {

struct Shocker {
  std::string id;
  std::string name;
  bool is_owned = false;
};

class OpenShockClient {
public:
  OpenShockClient();
  ~OpenShockClient();

  void SetToken(const std::string &token);
  bool HasToken() const { return !token_.empty(); }
  bool IsTokenValid() const { return token_valid_; }

  // Fetch list of shockers (owned and shared)
  bool FetchShockers();
  const std::vector<Shocker> &GetShockers() const { return shockers_; }

  // Control shockers
  // type: "Shock", "Vibrate", "Sound", "Stop"
  bool SendControl(const std::string &shocker_id, const std::string &type,
                   float intensity, int duration_ms);

private:
  bool ParseShockers(const std::string &json, bool is_owned);
  bool PerformPost(const std::string &url, const std::string &payload);
  bool PerformGet(const std::string &url, std::string &response);

  CURL *curl_ = nullptr;
  std::string token_;
  bool token_valid_ = false;
  std::vector<Shocker> shockers_;

  static constexpr const char *API_BASE = "https://api.openshock.app";
  static constexpr const char *USER_AGENT = "YipOS/1.0";
};

} // namespace YipOS

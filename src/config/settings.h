#pragma once
#include <string>
namespace settings {
void initialize();
std::string loadBaseUrl();
bool saveBaseUrl(const std::string &url);
struct WifiCredentials { char ssid[33]; char password[65]; };
bool loadWifi(WifiCredentials &credentials);
bool saveWifi(const WifiCredentials &credentials);
// Accept an HTTP origin (host and optional port), trimming spaces/trailing slash.
bool normalizeBaseUrl(const char *input, std::string &output);
}

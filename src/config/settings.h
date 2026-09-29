#pragma once
#include <string>
namespace settings {
void initialize();
std::string loadBaseUrl();
bool saveBaseUrl(const std::string &url);
// Accept an HTTP origin (host and optional port), trimming spaces/trailing slash.
bool normalizeBaseUrl(const char *input, std::string &output);
}

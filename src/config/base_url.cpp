#include "settings.h"
#include "project_config.h"
#include <cctype>
namespace settings {
bool normalizeBaseUrl(const char *input, std::string &output) {
    if (!input) return false;
    std::string url(input);
    const auto first = url.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    url = url.substr(first, url.find_last_not_of(" \t\r\n") - first + 1);
    while (!url.empty() && url.back() == '/') url.pop_back();
    if (url.size() >= config::URL_CAPACITY || url.compare(0, 7, "http://") != 0)
        return false;
    std::string host = url.substr(7);
    auto colon = host.find(':');
    if (colon != std::string::npos) {
        std::string port = host.substr(colon + 1);
        if (port.empty() || port.size() > 5) return false;
        unsigned number = 0;
        for (char c : port) {
            if (c < '0' || c > '9') return false;
            number = number * 10 + (c - '0');
        }
        if (number == 0 || number > 65535) return false;
        host.resize(colon);
    }
    if (host.empty() || host == "localhost") return false;
    // Keep the lesson to IPv4/hostname origins, with no paths or credentials.
    size_t labelLength = 0;
    char previous = '.';
    for (unsigned char c : host) {
        if (c == '.') {
            if (!labelLength || previous == '-') return false;
            labelLength = 0;
        } else {
            if (!(std::isalnum(c) || c == '-') || (!labelLength && c == '-')) return false;
            if (++labelLength > 63) return false;
        }
        previous = c;
    }
    if (!labelLength || previous == '-') return false;
    output = url;
    return true;
}
}

#pragma once
namespace wifi {
constexpr int MAX_NETWORKS = 12;
struct Network { char ssid[33]; bool secured; };
struct ScanResults { unsigned revision; int count; Network networks[MAX_NETWORKS]; char message[80]; };
struct Status { bool connected; char text[80]; };
void start();
void retry();
bool scan();
bool connect(const char *ssid, const char *password);
ScanResults scanResults();
Status status();
}

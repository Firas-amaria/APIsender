#pragma once
namespace wifi {
struct Status { bool connected; char text[80]; };
void start();
void retry();
Status status();
}

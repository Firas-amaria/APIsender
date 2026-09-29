#pragma once
namespace api {
struct Result { bool success = false; int number = 0; char message[128] = {}; };
// These blocking functions are called ONLY by the application's worker task.
Result checkHealth(const char *baseUrl);
Result getRandomNumber(const char *baseUrl);
Result sendColor(const char *baseUrl, const char *color);
Result sendEvent(const char *baseUrl, const char *button);
}

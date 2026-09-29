#pragma once
#include <cstdint>
#include "network/api_client.h"
#include "config/project_config.h"
namespace app {
enum class Action { Health, Random, Color, Event, Save, Test };
struct Completion { Action action; uint32_t view; api::Result result; char value[config::URL_CAPACITY]; };
void start();
const char *baseUrl();
// UI task owns these functions; the worker receives copies through queues.
bool submit(Action action, const char *value, uint32_t view);
bool poll(Completion &completion);
bool busy();
}

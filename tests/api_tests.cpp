// Compile the production API module against a deterministic fake transport.
#include "esp_http_client.h"
#include "network/api_client.h"
#include "network/wifi_manager.h"
#include "config/settings.h"
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <cerrno>
static esp_http_client_config_t options;
static std::string url, sent, accept, contentType, response;
static int method, statusCode, errorCode, socketError, calls;
static bool connected=true, allocationFails=false;
const char *esp_err_to_name(int) { return "test"; }
namespace wifi { Status status() { return {connected, "test"}; } }
void *esp_http_client_init(const esp_http_client_config_t *input) {
 options=*input; url=input->url; sent.clear(); accept.clear(); contentType.clear(); method=0;
 return allocationFails ? nullptr : &options;
}
int esp_http_client_set_header(void *,const char *key,const char *value) {
 if (!strcmp(key,"Accept")) accept=value;
 if (!strcmp(key,"Content-Type")) contentType=value;
 return 0;
}
int esp_http_client_set_method(void *,int value) { method=value; return 0; }
int esp_http_client_set_post_field(void *,const char *body,int length) { sent.assign(body,length); return 0; }
int esp_http_client_perform(void *) {
 ++calls;
 // Deliberately use many chunks, including an oversized response.
 for (size_t offset=0; offset<response.size(); offset+=7) {
   int length=static_cast<int>((response.size()-offset)<7 ? response.size()-offset : 7);
   esp_http_client_event_t event={HTTP_EVENT_ON_DATA,options.user_data,&response[offset],length};
   options.event_handler(&event);
 }
 return errorCode;
}
int esp_http_client_get_errno(void *) { return socketError; }
int esp_http_client_get_status_code(void *) { return statusCode; }
int esp_http_client_cleanup(void *) { return 0; }
static void reply(const char *body,int status=200,int error=0) {
 response=body; statusCode=status; errorCode=error; socketError=0;
}
int main() {
 const char *base="http://192.168.1.100:3000";
 reply("{\"status\":\"ok\"}");
 assert(api::checkHealth(base).success);
 assert(url==std::string(base)+"/api/health" && method==0 && sent.empty());
 assert(accept=="application/json" && options.disable_auto_redirect && options.timeout_ms==5000);
 for (const char *bad : {"{}","null","[]","{\"status\":true}","{\"status\":\"OK\"}","broken","{\"status\":\"ok\"} garbage"}) {
  reply(bad); auto result=api::checkHealth(base);
  assert(!result.success && !strcmp(result.message,"Invalid Response"));
 }
 for (int code : {201,204,301,404,500}) { reply("{\"status\":\"ok\"}",code); assert(!api::checkHealth(base).success); }
 reply("{\"number\":42}"); auto number=api::getRandomNumber(base);
 assert(number.success && number.number==42 && url==std::string(base)+"/api/random");
 for (int value : {0,-7,2147483647,(-2147483647-1)}) {
  std::string json="{\"number\":"+std::to_string(value)+"}";
  reply(json.c_str()); auto result=api::getRandomNumber(base); assert(result.success && result.number==value);
 }
 for (const char *bad : {"{\"number\":\"42\"}","{\"number\":1.5}","{\"number\":2147483648}","{\"number\":-2147483649}","{\"number\":1e999}","{\"number\":true}","{}","null"}) {
  reply(bad); assert(!api::getRandomNumber(base).success);
 }
 for (const char *color : {"red","green","blue","yellow"}) {
  reply("{\"success\":true}"); assert(api::sendColor(base,color).success);
  assert(url==std::string(base)+"/api/color" && method==HTTP_METHOD_POST);
  assert(sent==std::string("{\"color\":\"")+color+"\"}" && contentType=="application/json");
 }
 for (const char *button : {"A","B","C"}) {
  reply("",204); assert(api::sendEvent(base,button).success);
  assert(url==std::string(base)+"/api/event" && sent==std::string("{\"button\":\"")+button+"\"}");
 }
 reply("ignored",201); assert(api::sendColor(base,"red").success);
 reply("{}",400); assert(!api::sendEvent(base,"A").success);
 int previous=calls;
 assert(!api::sendColor(base,"purple").success && !api::sendEvent(base,"D").success && calls==previous);
 connected=false; assert(!strcmp(api::checkHealth(base).message,"Wi-Fi Disconnected") && calls==previous); connected=true;
 reply("",200,ESP_ERR_TIMEOUT); assert(!strcmp(api::checkHealth(base).message,"Request Timeout"));
 reply("",200,ESP_FAIL); socketError=ETIMEDOUT; assert(!strcmp(api::checkHealth(base).message,"Request Timeout"));
 reply("",200,ESP_FAIL); assert(!strcmp(api::checkHealth(base).message,"Server Unreachable"));
 reply("",200); response=std::string(1025,'x'); assert(!strcmp(api::checkHealth(base).message,"Response Too Large"));
 reply("",200); response=std::string(40,'[')+"0"+std::string(40,']'); assert(!api::getRandomNumber(base).success);
 allocationFails=true; assert(!api::checkHealth(base).success); allocationFails=false;
 std::string normalized;
 for (const char *good : {base," http://server:8080/ \n","http://example.com","http://my-server:65535"}) assert(settings::normalizeBaseUrl(good,normalized));
 assert(settings::normalizeBaseUrl(" http://192.168.1.100:3000/// ",normalized) && normalized==base);
 for (const char *bad : {""," ","http://","https://server","server:3000","http://localhost:3000","http://host:0","http://host:65536","http://host:abc","http://host/api","http://host?q=1","http://user@host","http://ho st","http://-host","http://host-","http://host..com","http://[::1]:80"}) assert(!settings::normalizeBaseUrl(bad,normalized));
 assert(!settings::normalizeBaseUrl(nullptr,normalized));
 assert(!settings::normalizeBaseUrl((std::string("http://")+std::string(200,'a')).c_str(),normalized));
 std::cout << "PASS: production API contract, JSON, errors, chunk limits and URL validation\n";
}

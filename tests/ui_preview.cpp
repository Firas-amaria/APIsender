// Render the real UI and LVGL to images; no ESP32 or display driver is emulated.
#include "ui/ui.h"
#include "app/app.h"
#include "network/wifi_manager.h"
#include "lvgl.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
static lv_color_t pixels[320*480], drawBuffer[320*40];
namespace app {
const char *baseUrl() { return "http://192.168.1.100:3000"; }
bool busy() { return false; }
bool submit(Action,const char *,uint32_t) { return true; }
bool poll(Completion &) { return false; }
}
namespace wifi {
Status status() { return {true,"Wi-Fi: Connected\nIP: 192.168.1.50"}; }
void retry() {}
}
static void flush(lv_disp_drv_t *driver,const lv_area_t *area,lv_color_t *colors) {
 for (int y=area->y1;y<=area->y2;++y)
  for (int x=area->x1;x<=area->x2;++x) {
   if(x>=0 && x<320 && y>=0 && y<480) pixels[y*320+x]=*colors;
   ++colors;
  }
 lv_disp_flush_ready(driver);
}
static lv_obj_t *findText(lv_obj_t *root,const char *text) {
 if(lv_obj_check_type(root,&lv_label_class) && !strcmp(lv_label_get_text(root),text)) return root;
 for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i)
  if(auto found=findText(lv_obj_get_child(root,i),text)) return found;
 return nullptr;
}
static lv_obj_t *findType(lv_obj_t *root,const lv_obj_class_t *type) {
 if(lv_obj_check_type(root,type)) return root;
 for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i)
  if(auto found=findType(lv_obj_get_child(root,i),type)) return found;
 return nullptr;
}
static void click(const char *text) {
 auto caption=findText(lv_scr_act(),text); assert(caption);
 auto button=lv_obj_get_parent(caption);
 assert(lv_obj_check_type(button,&lv_btn_class));
 lv_event_send(button,LV_EVENT_CLICKED,nullptr);
}
static void snapshot(const char *name) {
 lv_tick_inc(120); lv_timer_handler(); lv_refr_now(nullptr);
 std::ofstream file(std::string("tests/build/")+name+".ppm",std::ios::binary);
 file << "P6\n320 480\n255\n";
 for(auto color:pixels) {
  lv_color32_t rgb; rgb.full=lv_color_to32(color);
  char channels[3]={static_cast<char>(rgb.ch.red),static_cast<char>(rgb.ch.green),static_cast<char>(rgb.ch.blue)};
  file.write(channels,3);
 }
}
int main() {
 lv_init();
 lv_disp_draw_buf_t buffer; lv_disp_draw_buf_init(&buffer,drawBuffer,nullptr,320*40);
 lv_disp_drv_t driver; lv_disp_drv_init(&driver);
 driver.hor_res=320; driver.ver_res=480; driver.draw_buf=&buffer; driver.flush_cb=flush;
 lv_disp_drv_register(&driver);
 ui::start(); snapshot("menu");
 for(auto page : {"Health Check","Get Random Number","Send Color","Send Event","Settings"}) {
  click(page); snapshot(page);
  if(!strcmp(page,"Settings")) {
   auto textarea=findType(lv_scr_act(),&lv_textarea_class); assert(textarea);
   lv_event_send(textarea,LV_EVENT_CLICKED,nullptr); snapshot("keyboard");
   auto keyboard=findType(lv_scr_act(),&lv_keyboard_class); assert(keyboard);
   assert(!lv_obj_has_flag(keyboard,LV_OBJ_FLAG_HIDDEN));
   lv_event_send(keyboard,LV_EVENT_READY,nullptr);
   assert(lv_obj_has_flag(keyboard,LV_OBJ_FLAG_HIDDEN));
  }
  click("Back");
 }
 std::cout << "PASS: real LVGL renders six screens; navigation and keyboard events work\n";
}


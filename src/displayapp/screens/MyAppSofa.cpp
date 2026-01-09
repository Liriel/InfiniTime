#include "displayapp/screens/MyAppSofa.h"
#include <cstdint>
#include "components/ble/RemoteControlService.h"

using namespace Pinetime::Applications::Screens;

static void event_handler(lv_obj_t* obj, lv_event_t event) {
  MyAppSofa* screen = static_cast<MyAppSofa*>(obj->user_data);
  screen->OnObjectEvent(obj, event);
}

MyAppSofa::MyAppSofa(Pinetime::Controllers::RemoteControlService& remoteControl) 
  : remoteControlService(remoteControl) {
  
  title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(title, "Sofa");
  lv_obj_align(title, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);

  label1 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(label1, "Lamp 1: 0");
  lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 60);

  slider1 = lv_slider_create(lv_scr_act(), nullptr);
  slider1->user_data = this;
  lv_obj_set_width(slider1, LV_HOR_RES - 20);
  lv_obj_align(slider1, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 90);
  lv_obj_set_event_cb(slider1, event_handler);
  lv_slider_set_range(slider1, 0, 100);

  label2 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(label2, "Lamp 2: 0");
  lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 140);

  slider2 = lv_slider_create(lv_scr_act(), nullptr);
  slider2->user_data = this;
  lv_obj_set_width(slider2, LV_HOR_RES - 20);
  lv_obj_align(slider2, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 170);
  lv_obj_set_event_cb(slider2, event_handler);
  lv_slider_set_range(slider2, 0, 100);
}

MyAppSofa::~MyAppSofa() {
  lv_obj_clean(lv_scr_act());
}

void MyAppSofa::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_VALUE_CHANGED) {
    if (obj == slider1) {
      uint8_t val = lv_slider_get_value(slider1);
      lv_label_set_text_fmt(label1, "Lamp 1: %d", val);
      uint8_t data[2] = {0x04, val};
      remoteControlService.DeviceEvent(data, 2);
    } else if (obj == slider2) {
      uint8_t val = lv_slider_get_value(slider2);
      lv_label_set_text_fmt(label2, "Lamp 2: %d", val);
      uint8_t data[2] = {0x05, val};
      remoteControlService.DeviceEvent(data, 2);
    }
  }
}

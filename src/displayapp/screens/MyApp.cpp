#include "displayapp/screens/MyApp.h"
#include <cstdint>
#include "components/ble/RemoteControlService.h"

using namespace Pinetime::Applications::Screens;

static void event_handler(lv_obj_t* obj, lv_event_t event) {
  MyApp* screen = static_cast<MyApp*>(obj->user_data);
  screen->OnObjectEvent(obj, event);
}

MyApp::MyApp(Pinetime::Controllers::RemoteControlService& remoteControl) : remoteControlService(remoteControl) {
  /* title */
  //lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  //lv_label_set_text_static(title, "Test");
  title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(title, "Test");
  lv_obj_align(title, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);

  slider = lv_slider_create(lv_scr_act(), nullptr);
  slider->user_data = this;
  lv_obj_set_width(slider, (LV_DPI * 2) - 6);
  lv_obj_align(slider, lv_scr_act(), LV_ALIGN_CENTER, 0, 70);
  lv_obj_set_event_cb(slider, event_handler);
  lv_slider_set_range(slider, 0, 100);

  btnVolDown = lv_btn_create(lv_scr_act(), nullptr);
  btnVolDown->user_data = this;
  lv_obj_set_event_cb(btnVolDown, event_handler);
  lv_obj_set_size(btnVolDown, 76, 76);
  lv_obj_align(btnVolDown, lv_scr_act(), LV_ALIGN_CENTER, 0, -20);
  btnlabel = lv_label_create(btnVolDown, nullptr);
  lv_label_set_text(btnlabel, "!");

  /* label  
  label = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(label, "Hello");
  //lv_label_set_text_static(label, "My test application");
  lv_obj_set_auto_realign(label, true);
  lv_obj_align(label, slider, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
  */

  
}

MyApp::~MyApp() {
  lv_obj_clean(lv_scr_act());
}

void MyApp::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED && obj == btnVolDown) {
    lv_label_set_text(title, "hi");
    const uint8_t val = 5;
    const char* msg = remoteControlService.ButtonEvent(&val);
    lv_label_set_text(title, msg);
  }
  else if(event == LV_EVENT_VALUE_CHANGED && obj == slider) {
    uint8_t val = lv_slider_get_value(slider);
    //lv_label_set_text_fmt(label, "%03d", val);
    lv_label_set_text_fmt(title, "Slider: %03d", val);
    remoteControlService.SliderEvent(&val);
  }
}


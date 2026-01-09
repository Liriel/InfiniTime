#include "displayapp/screens/DoorControl.h"
#include <cstdint>
#include "components/ble/RemoteControlService.h"

using namespace Pinetime::Applications::Screens;

static void event_handler(lv_obj_t* obj, lv_event_t event) {
  DoorControl* screen = static_cast<DoorControl*>(obj->user_data);
  screen->OnObjectEvent(obj, event);
}

DoorControl::DoorControl(Pinetime::Controllers::RemoteControlService& remoteControl) : remoteControlService(remoteControl) {
  title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(title, "Door Control");
  lv_obj_align(title, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);

  btnDoor = lv_btn_create(lv_scr_act(), nullptr);
  btnDoor->user_data = this;
  lv_obj_set_event_cb(btnDoor, event_handler);
  lv_obj_set_size(btnDoor, 120, 120);
  lv_obj_align(btnDoor, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  btnLabel = lv_label_create(btnDoor, nullptr);
  lv_label_set_text(btnLabel, "OPEN");

  statusLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(statusLabel, "Ready");
  lv_obj_align(statusLabel, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -20);
}

DoorControl::~DoorControl() {
  lv_obj_clean(lv_scr_act());
}

void DoorControl::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED && obj == btnDoor) {
    const uint8_t deviceId = 0x01;
    const char* msg = remoteControlService.ButtonEvent(&deviceId);
    lv_label_set_text(statusLabel, msg);
  }
}

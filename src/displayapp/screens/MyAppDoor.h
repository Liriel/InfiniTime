#pragma once

#include <FreeRTOS.h>
#include <lvgl/src/lv_core/lv_obj.h>
#include "displayapp/screens/Screen.h"

namespace Pinetime {
  namespace Controllers {
    class RemoteControlService;
  }

  namespace Applications {
    namespace Screens {
      class MyAppDoor : public Screen {
      public:
        MyAppDoor(Pinetime::Controllers::RemoteControlService& remoteControl);
        ~MyAppDoor() override;
        void OnObjectEvent(lv_obj_t* obj, lv_event_t event);

      private:
        lv_obj_t* btnDoor;
        lv_obj_t* btnLabel;
        lv_obj_t* title;
        lv_obj_t* statusLabel;
        Pinetime::Controllers::RemoteControlService& remoteControlService;
      };
    }
  }
}

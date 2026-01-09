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
      class MyAppSofa : public Screen {
      public:
        MyAppSofa(Pinetime::Controllers::RemoteControlService& remoteControl);
        ~MyAppSofa() override;
        void OnObjectEvent(lv_obj_t* obj, lv_event_t event);

      private:
        lv_obj_t* slider1;
        lv_obj_t* slider2;
        lv_obj_t* label1;
        lv_obj_t* label2;
        lv_obj_t* title;
        Pinetime::Controllers::RemoteControlService& remoteControlService;
      };
    }
  }
}

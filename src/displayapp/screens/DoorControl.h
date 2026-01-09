#pragma once

#include <FreeRTOS.h>
#include <lvgl/src/lv_core/lv_obj.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "systemtask/SystemTask.h"

namespace Pinetime {
  namespace Controllers {
    class RemoteControlService;
  }

  namespace Applications {
    namespace Screens {
      class DoorControl : public Screen {
      public:
        DoorControl(Pinetime::Controllers::RemoteControlService& remoteControl);
        ~DoorControl() override;
        void OnObjectEvent(lv_obj_t* obj, lv_event_t event);
      private:
        lv_obj_t* btnDoor;
        lv_obj_t* btnLabel;
        lv_obj_t* title;
        lv_obj_t* statusLabel;
        Pinetime::Controllers::RemoteControlService& remoteControlService;
      };
    }

    template <>
    struct AppTraits<Apps::DoorControl> {
      static constexpr Apps app = Apps::DoorControl;
      static constexpr const char* icon = "D";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::DoorControl(controllers.systemTask->nimble().remoteControl());
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}

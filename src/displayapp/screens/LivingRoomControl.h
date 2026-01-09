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
      class LivingRoomControl : public Screen {
      public:
        LivingRoomControl(Pinetime::Controllers::RemoteControlService& remoteControl);
        ~LivingRoomControl() override;
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

    template <>
    struct AppTraits<Apps::LivingRoomControl> {
      static constexpr Apps app = Apps::LivingRoomControl;
      static constexpr const char* icon = "L";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::LivingRoomControl(controllers.systemTask->nimble().remoteControl());
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}

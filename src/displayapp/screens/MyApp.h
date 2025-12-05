#pragma once

#include <FreeRTOS.h>
#include <lvgl/src/lv_core/lv_obj.h>
#include <string>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"

namespace Pinetime {
  namespace Controllers {
    class RemoteControlService;
  }

  namespace Applications {
    namespace Screens {
      class MyApp : public Screen {
      public:
        MyApp(Pinetime::Controllers::RemoteControlService& remoteControl);
        ~MyApp() override;
        void OnObjectEvent(lv_obj_t* obj, lv_event_t event);
      private:
        lv_obj_t* slider;
        lv_obj_t* btnVolDown;
        lv_obj_t* btnlabel;
        lv_obj_t* label;
        lv_obj_t* title;
        Pinetime::Controllers::RemoteControlService& remoteControlService;
      };
    }

    template <>
    struct AppTraits<Apps::MyApp> {
      static constexpr Apps app = Apps::MyApp;
      static constexpr const char* icon = "M";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::MyApp(controllers.systemTask->nimble().remoteControl());
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}

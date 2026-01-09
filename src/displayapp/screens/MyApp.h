#pragma once

#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "systemtask/SystemTask.h"

namespace Pinetime {
  namespace Controllers {
    class RemoteControlService;
  }

  namespace Applications {
    class DisplayApp;

    namespace Screens {
      class MyApp : public Screen {
      public:
        MyApp(DisplayApp* app, Pinetime::Controllers::RemoteControlService& remoteControl);
        ~MyApp() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        DisplayApp* app;
        Pinetime::Controllers::RemoteControlService& remoteControlService;
        ScreenList<3> screens;

        std::unique_ptr<Screen> CreateDoorScreen();
        std::unique_ptr<Screen> CreateLivingRoomScreen();
        std::unique_ptr<Screen> CreateSofaScreen();
      };
    }

    template <>
    struct AppTraits<Apps::MyApp> {
      static constexpr Apps app = Apps::MyApp;
      static constexpr const char* icon = "M";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::MyApp(controllers.displayApp, 
                                   controllers.systemTask->nimble().remoteControl());
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}

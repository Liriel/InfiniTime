#pragma once

#include <array>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include "displayapp/screens/List.h"
#include "displayapp/apps/Apps.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      class HomeControl : public Screen {
      public:
        HomeControl(DisplayApp* app, Pinetime::Controllers::Settings& settingsController);
        ~HomeControl() override;

        bool OnTouchEvent(Pinetime::Applications::TouchEvents event) override;

      private:
        DisplayApp* app;
        auto CreateScreenList() const;
        std::unique_ptr<Screen> CreateScreen(unsigned int screenNum) const;

        Controllers::Settings& settingsController;

        static constexpr int entriesPerScreen = 4;
        static constexpr int nScreens = 1;

        static constexpr std::array<List::Applications, entriesPerScreen * nScreens> entries {{
          {"D", "Door", Apps::DoorControl},
          {"L", "Living Room", Apps::LivingRoomControl},
          {"S", "Sofa", Apps::SofaControl},
          {"", "", Apps::None}
        }};
        ScreenList<nScreens> screens;
      };
    }
  }
}

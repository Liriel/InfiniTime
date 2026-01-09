#include "displayapp/screens/HomeControl.h"
#include <lvgl/lvgl.h>
#include <functional>
#include "displayapp/apps/Apps.h"
#include "displayapp/DisplayApp.h"

using namespace Pinetime::Applications::Screens;

constexpr std::array<List::Applications, HomeControl::entries.size()> HomeControl::entries;

auto HomeControl::CreateScreenList() const {
  std::array<std::function<std::unique_ptr<Screen>()>, nScreens> screens;
  for (size_t i = 0; i < screens.size(); i++) {
    screens[i] = [this, i]() -> std::unique_ptr<Screen> {
      return CreateScreen(i);
    };
  }
  return screens;
}

HomeControl::HomeControl(Pinetime::Applications::DisplayApp* app, Pinetime::Controllers::Settings& settingsController)
  : app {app},
    settingsController {settingsController},
    screens {app, 0, CreateScreenList(), Screens::ScreenListModes::UpDown} {
}

HomeControl::~HomeControl() {
  lv_obj_clean(lv_scr_act());
}

bool HomeControl::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  return screens.OnTouchEvent(event);
}

std::unique_ptr<Screen> HomeControl::CreateScreen(unsigned int screenNum) const {
  std::array<List::Applications, entriesPerScreen> screens;
  for (int i = 0; i < entriesPerScreen; i++) {
    screens[i] = entries[screenNum * entriesPerScreen + i];
  }

  return std::make_unique<Screens::List>(screenNum, nScreens, app, settingsController, screens);
}

#include "displayapp/screens/MyApp.h"
#include "displayapp/screens/MyAppDoor.h"
#include "displayapp/screens/MyAppLivingRoom.h"
#include "displayapp/screens/MyAppSofa.h"
#include "displayapp/DisplayApp.h"

using namespace Pinetime::Applications::Screens;

MyApp::MyApp(DisplayApp* app, Pinetime::Controllers::RemoteControlService& remoteControl)
  : app {app},
    remoteControlService {remoteControl},
    screens {app,
             0,
             {[this]() -> std::unique_ptr<Screen> {
                return CreateDoorScreen();
              },
              [this]() -> std::unique_ptr<Screen> {
                return CreateLivingRoomScreen();
              },
              [this]() -> std::unique_ptr<Screen> {
                return CreateSofaScreen();
              }},
             ScreenListModes::UpDown} {
}

MyApp::~MyApp() {
  lv_obj_clean(lv_scr_act());
}

bool MyApp::OnTouchEvent(TouchEvents event) {
  return screens.OnTouchEvent(event);
}

std::unique_ptr<Screen> MyApp::CreateDoorScreen() {
  return std::make_unique<MyAppDoor>(remoteControlService);
}

std::unique_ptr<Screen> MyApp::CreateLivingRoomScreen() {
  return std::make_unique<MyAppLivingRoom>(remoteControlService);
}

std::unique_ptr<Screen> MyApp::CreateSofaScreen() {
  return std::make_unique<MyAppSofa>(remoteControlService);
}


# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**LassiBLEHero** is a customized InfiniTime firmware for the PineTime smartwatch. This is a fork of the upstream InfiniTime project with custom BLE remote control capabilities.

### Customizations in lassi-ble-hero Branch

This branch adds a custom BLE Remote Control Service that enables the watch to send UI events (button presses, slider changes) to connected BLE devices. Key additions:

- **RemoteControlService** - New BLE service (`00060000-78fc-48fe-8e23-433b3a1942d0`) with three characteristics:
  - Button Event (`00060200-...`) - Notifies on button presses
  - Slider Event (`00060100-...`) - Notifies on slider value changes
  - Status (`00060300-...`) - Read/write characteristic for status messages

- **MyApp** - Demo application showcasing the remote control functionality with:
  - Interactive button that sends BLE notifications when pressed
  - Slider that streams value changes via BLE
  - Integration with RemoteControlService

- **bluez_test.py** - Python test script for Linux/BlueZ to connect and monitor BLE notifications from the watch

- **Branding** - Device name changed to "LassiBLEHero", Terminal watchface customized with "s7arbuck" branding

The base InfiniTime firmware runs on the nRF52832 SoC (ARM Cortex-M4) using FreeRTOS as its real-time operating system, with UI built on LVGL and BLE via NimBLE.

## Build Commands
build using Docker like this:

```bash
docker run --rm -it -v ${PWD}:/sources infinitime-build
```

results are stored in `build/output/`

### Code Quality

```bash
# Format code (use before committing)
clang-format -i src/**/*.cpp src/**/*.h

# Check code quality
clang-tidy src/**/*.cpp
```

## Architecture Overview

### FreeRTOS Task Structure

InfiniTime uses FreeRTOS for multitasking. Key tasks:

- **MAIN task** (`SystemTask::Work()` in `src/systemtask/SystemTask.cpp`)
  - Primary task that initializes all drivers and controllers
  - Manages system state (running/sleeping)
  - Coordinates between different subsystems

- **displayapp task** (`DisplayApp` in `src/displayapp/DisplayApp.cpp`)
  - Handles UI rendering, app lifecycle, and user input
  - Runs apps and watch faces
  - Processes touch events and forwards to active app

- **BLE tasks** ("ll" and "ble")
  - Handle Bluetooth Low Energy communication
  - Located in NimBLE library code

- **HeartRateTask** (`src/heartratetask/HeartRateTask.cpp`)
  - Periodic heart rate measurements

### Controller Pattern

Controllers are singleton objects that provide access to system resources. They are:
- Declared in `main.cpp`
- Initialized in `SystemTask::Work()`
- Located in `src/components/` subdirectories
- Passed by reference to apps that need them

Key controllers:
- `Battery` - Battery level monitoring
- `DateTime` - Time and date management
- `Ble` / `NimbleController` - Bluetooth functionality
- `NotificationManager` - Notification handling
- `MotionController` - Accelerometer data
- `HeartRateController` - Heart rate sensor
- `Settings` - Persistent settings storage
- `AlarmController` - Alarm functionality
- `FS` - Filesystem (LittleFS on external SPI flash)

### Application Architecture

All apps are in the `Pinetime::Applications::Screens` namespace and inherit from `Screen` (`src/displayapp/screens/Screen.h`).

**Three types of applications:**

1. **System apps** - Always built-in, required for core functionality (Settings, Notifications, Launcher)
2. **User apps** - Optional, selectable at build time via `ENABLE_USERAPPS`
3. **Watch faces** - Optional, at least one required, selectable via `ENABLE_WATCHFACES`

**App lifecycle:**
- Created when launched by `DisplayApp::LoadScreen()`
- Constructor initializes LVGL UI elements
- Destructor cleans up LVGL objects with `lv_obj_clean(lv_scr_act())`
- Instance destroyed when user exits

**App registration:**

User apps and watch faces use a template-based registration system:
- Define `AppTraits<Apps::YourApp>` with icon and `Create()` function
- Add app enum to `Apps.h`
- Add to `ENABLE_USERAPPS` or `ENABLE_WATCHFACES` CMake variable
- Automatically appears in app launcher

System apps are handled directly in `DisplayApp::LoadScreen()` switch statement.

### UI Framework (LVGL)

InfiniTime uses LVGL 7.x for the user interface:
- UI elements are created in app constructors using `lv_*` functions
- Screen resolution: 240x240 pixels
- Touch input handled through `OnTouchEvent()` methods
- Periodic refresh via `lv_task_create()` calling `Refresh()` method

### BLE Architecture

Bluetooth is handled by the NimBLE stack (`src/components/ble/`):
- `NimbleController` manages BLE connections and services
- Standard services: CTS (Current Time), ANS (Alert Notification), DIS (Device Info)
- Custom services: Music Control, Navigation, Motion, Simple Weather
- Custom service UUID pattern: `xxxxxxxx-78fc-48fe-8e23-433b3a1942d0`
- Service implementations in `src/components/ble/`

### File Organization

```
src/
├── main.cpp                    # Entry point, creates SystemTask
├── systemtask/                 # System task and main control loop
├── displayapp/                 # Display task and UI management
│   ├── screens/                # All app implementations
│   ├── apps/                   # App metadata (Apps.h)
│   ├── fonts/                  # Font files and generation
│   └── icons/                  # Icon resources
├── components/                 # Controllers for system resources
│   ├── ble/                    # BLE services and controller
│   ├── battery/                # Battery monitoring
│   ├── datetime/               # Time/date management
│   ├── motion/                 # Motion sensor controller
│   ├── heartrate/              # Heart rate controller
│   ├── settings/               # Settings storage
│   ├── alarm/                  # Alarm controller
│   ├── fs/                     # Filesystem (LittleFS)
│   └── ...
├── drivers/                    # Hardware drivers
│   ├── Cst816S.*               # Touch controller
│   ├── St7789.*                # Display driver
│   ├── Hrs3300.*               # Heart rate sensor
│   ├── Bma421.*                # Accelerometer
│   └── ...
├── libs/                       # Third-party libraries
│   ├── lvgl/                   # UI library
│   ├── mynewt-nimble/          # BLE stack
│   ├── littlefs/               # Filesystem
│   └── FreeRTOS/               # RTOS
└── resources/                  # External resources (fonts, images)
```

## Development Guidelines

### Coding Conventions

Follow guidelines in `doc/coding-convention.md`:
- **Indentation**: 2 spaces, no tabs
- **Braces**: Opening brace at end of line
- **Naming**:
  - Classes/namespaces: PascalCase
  - Variables: camelCase (no prefixes/suffixes for members)
- **Include guards**: Use `#pragma once`
- **Includes**:
  - Project files: `#include "relative/path/from/src/file.h"`
  - External/std: `#include <file.h>`
- Use `nullptr` instead of `NULL`
- Use `auto` sparingly, not for fundamental types
- Format with `clang-format` before committing
- Check with `clang-tidy`

### Commit Format

```
module: Short description

A more thorough description of all changes if necessary.
```

Where module is a file name or scope (e.g., "DisplayApp", "BLE", "Alarm").

### Creating a New App

1. Create `MyApp.h` and `MyApp.cpp` in `src/displayapp/screens/`
2. Inherit from `Screen` class
3. Define `AppTraits<Apps::MyApp>` template in header
4. Add enum entry to `Apps.h`
5. Add to CMakeLists.txt
6. Include in `ENABLE_USERAPPS` CMake variable

Constructor should initialize LVGL UI, destructor should call `lv_obj_clean(lv_scr_act())`.

### Memory Constraints

- Target device has limited RAM (64KB) and Flash (512KB)
- Release builds use size/speed optimizations
- Debug builds may exceed flash capacity - selectively debug specific targets
- Estimate stack size for new tasks (`configMINIMAL_STACK_SIZE` = 120 words)

### Testing

- Test on actual PineTime hardware or InfiniSim simulator
- Verify firmware in Settings after OTA update
- Check memory usage with `doc/MemoryAnalysis.md` techniques

## Common Tasks

### Adding a new BLE characteristic

1. Define characteristic in appropriate service file in `src/components/ble/`
2. Follow UUID pattern for custom services
3. Implement characteristic callbacks
4. Document in `doc/ble.md`

### Adding a new controller

1. Create controller in `src/components/<name>/`
2. Declare instance in `main.cpp`
3. Initialize in `SystemTask::Work()`
4. Pass to apps via `AppControllers` struct if needed

### Modifying watch face selection

Available watch faces are generated at compile time. To add/remove:
- Define `WatchFaceTraits<WatchFace::YourFace>` template
- Modify `ENABLE_WATCHFACES` CMake variable

### Working with LittleFS

- Filesystem resides on external SPI flash
- Access via `Controllers::FS` controller
- Used for settings, resources, and external fonts
- See `src/components/fs/` for implementation

## LassiBLEHero Specific Features

### Remote Control Service Architecture

The RemoteControlService (`src/components/ble/RemoteControlService.{h,cpp}`) provides a BLE GATT service for sending UI events from the watch to connected devices.

**Service UUID**: `00060000-78fc-48fe-8e23-433b3a1942d0`

**Characteristics**:
- **Slider Event** (`00060100-...`): Notify-only, sends uint8_t slider values (0-100)
- **Button Event** (`00060200-...`): Notify-only, sends uint8_t button event codes
- **Status** (`00060300-...`): Read/write, accepts string status messages from connected device

**Integration**:
1. Service instance created in `NimbleController` constructor
2. Initialized in `NimbleController::Init()`
3. Accessed by apps via `systemTask.nimble().remoteControl()`

### MyApp Implementation

MyApp (`src/displayapp/screens/MyApp.{h,cpp}`) demonstrates remote control usage:

**UI Elements**:
- Title label - Shows connection status and slider value
- Button - Sends button event (value: 5) when clicked
- Slider - Continuously sends value changes (0-100)

**Key Methods**:
- `OnObjectEvent()` - Handles LVGL events and calls RemoteControlService methods
- `ButtonEvent(&val)` - Sends button notification via BLE
- `SliderEvent(&val)` - Sends slider notification via BLE

**Registration**:
- Added to `Apps` enum in `Apps.h.in`
- AppTraits defined in `MyApp.h`
- Icon: "M"
- Automatically added to app launcher

### Testing with bluez_test.py

Python script for testing BLE notifications on Linux systems using BlueZ:

```bash
# Run after pairing watch with bluetoothctl
python3 bluez_test.py
```

**What it does**:
1. Discovers RemoteControlService by UUID
2. Finds Button Event characteristic
3. Enables notifications
4. Prints received button/slider events in hex format
5. Gracefully disconnects on CTRL+C

**Requirements**:
- `python3-dbus`
- `python3-gi` (GObject introspection)
- BlueZ stack
- Watch already paired via `bluetoothctl`

### Customization Points

**Changing device name**: Edit `NimbleController.h:91`
```cpp
static constexpr const char* deviceName = "LassiBLEHero";
```

**Adding new event types**:
1. Define new characteristic in `RemoteControlService`
2. Add UUID following pattern: `0006CCCC-78fc-48fe-8e23-433b3a1942d0`
3. Implement notification method similar to `ButtonEvent()`/`SliderEvent()`
4. Call from app UI event handlers

**Terminal watchface customization**: `WatchFaceTerminal.cpp:46`
- Name displayed at top of watchface
- Font size changed to jetbrains_mono_42 for time display
- Simplified layout removes heartbeat/steps (commented out)

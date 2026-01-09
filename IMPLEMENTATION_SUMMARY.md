# Multi-Device Control Implementation

## Overview
Transformed the single MyApp screen into a hierarchical multi-device control system with:
- Central HomeControl hub menu
- DoorControl screen (button)
- LivingRoomControl screen (2 sliders)
- SofaControl screen (2 sliders)

## Files Created

### Screen Files (8 files)
1. **src/displayapp/screens/HomeControl.h** - Central menu screen header
2. **src/displayapp/screens/HomeControl.cpp** - Central menu implementation
3. **src/displayapp/screens/DoorControl.h** - Door button control header
4. **src/displayapp/screens/DoorControl.cpp** - Door button implementation
5. **src/displayapp/screens/LivingRoomControl.h** - Living room 2-slider header
6. **src/displayapp/screens/LivingRoomControl.cpp** - Living room implementation
7. **src/displayapp/screens/SofaControl.h** - Sofa 2-slider header
8. **src/displayapp/screens/SofaControl.cpp** - Sofa implementation

## Files Modified

### Core Application Files
1. **src/displayapp/apps/Apps.h.in**
   - Added: `HomeControl`, `DoorControl`, `LivingRoomControl`, `SofaControl` enums

2. **src/displayapp/DisplayApp.cpp**
   - Added includes for new screens
   - Added switch cases for all 4 new apps in `LoadScreen()`

3. **src/displayapp/UserApps.h**
   - Added: `#include "displayapp/screens/HomeControl.h"`

### BLE Service
4. **src/components/ble/RemoteControlService.h**
   - Added: `DeviceEvent(const uint8_t* data, size_t len)` method

5. **src/components/ble/RemoteControlService.cpp**
   - Implemented: `DeviceEvent()` to send multi-byte notifications

### Build Configuration
6. **src/CMakeLists.txt**
   - Added 4 .cpp files to source list
   - Added 4 .h files to header list

7. **src/displayapp/apps/CMakeLists.txt**
   - Added: `Apps::HomeControl` to default user apps

## Architecture

### Device ID Protocol
BLE notifications now use 2-byte format: `[device_id, value]`

Device IDs:
- `0x01` - Door button
- `0x02` - Living Room Lamp 1 (slider)
- `0x03` - Living Room Lamp 2 (slider)
- `0x04` - Sofa Lamp 1 (slider)
- `0x05` - Sofa Lamp 2 (slider)

### Navigation Flow
```
App Launcher → HomeControl (menu)
                ├─ DoorControl
                ├─ LivingRoomControl
                └─ SofaControl
```

### UI Components

**HomeControl:**
- List-style menu with 3 entries
- Uses Settings-style navigation pattern
- Icons: D, L, S

**DoorControl:**
- Single large "OPEN" button (120x120)
- Status label shows "done" or "no conn"
- Sends device_id=0x01, value=0x01 on click

**LivingRoomControl & SofaControl:**
- Title label ("Living Room" / "Sofa")
- 2 sliders (range 0-100)
- Labels show current values "Lamp 1: XX" / "Lamp 2: XX"
- Continuously sends values via BLE on slider change

## Build Instructions

1. Reconfigure CMake (only if build directory exists):
```bash
cd build
cmake ..
```

2. Build:
```bash
make -j4 pinetime-app
```

3. Flash firmware to watch

4. HomeControl app will appear in app launcher with "H" icon (if default apps used) or can be launched via Apps::HomeControl

## Testing

Update `bluez_test.py` to handle 2-byte notifications:
- First byte = device ID
- Second byte = value

## Notes

- MyApp remains functional and separate
- All screens properly clean up LVGL objects in destructors
- RemoteControlService backward compatible (old single-byte methods still work)
- HomeControl uses same List/ScreenList pattern as Settings

## Updated BLE Test Script

The `bluez_test.py` has been enhanced to:
- Subscribe to both Button (0x00060200) and Slider (0x00060100) characteristics
- Parse 2-byte notifications: [device_id, value]
- Display human-readable device names:
  - 0x01: Door
  - 0x02: Living Room Lamp 1
  - 0x03: Living Room Lamp 2
  - 0x04: Sofa Lamp 1
  - 0x05: Sofa Lamp 2
- Backward compatible with legacy single-byte format

Example output:
```
Door: 1
Living Room Lamp 1: 45
Sofa Lamp 2: 89
```

## Quick Test

After flashing firmware:
1. Pair watch via `bluetoothctl`
2. Run: `python3 bluez_test.py`
3. Open HomeControl app on watch
4. Navigate to any control screen
5. Interact with buttons/sliders
6. See real-time events in terminal

## Summary

**8 new files created**, **7 files modified**, implementing a complete hierarchical multi-device control system with proper BLE communication protocol and backward compatibility.

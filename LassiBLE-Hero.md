# LassiBLEHero - BLE Remote Control Documentation

## Overview

LassiBLEHero is a custom InfiniTime firmware for PineTime that adds a BLE Remote Control Service. This service enables the watch to send UI events (button presses and slider changes) to connected BLE clients in real-time.

**Device Name:** `LassiBLEHero`

**Firmware Version:** 1.15.0 (based on InfiniTime)

**Custom Features:**
- BLE Remote Control Service for sending watch UI events
- MyApp - Multi-screen home automation control with:
  - Door Control screen (button trigger)
  - Living Room screen (2 lamp sliders)
  - Sofa Area screen (2 lamp sliders)
- Terminal watchface with "s7arbuck" branding

---

## BLE Service Architecture

### Remote Control Service

**Service UUID:** `00060000-78fc-48fe-8e23-433b3a1942d0`

**Service Type:** Primary GATT Service

**Purpose:** Sends notifications when the user interacts with MyApp's UI elements (button presses and slider movements). Each notification includes a device ID to identify which control was activated.

---

## Characteristics

### 1. Slider Event Characteristic

**UUID:** `00060100-78fc-48fe-8e23-433b3a1942d0`

**Properties:**
- `READ` - Can read current slider value
- `NOTIFY` - Sends notifications when slider value changes

**Data Format:**
- Type: `uint8_t[]` (2 bytes)
- Byte 0: Device ID (0x02 for Lamp 1, 0x03 for Lamp 2, etc.)
- Byte 1: Value (0-100)
- Encoding: Little-endian

**Behavior:**
- Notification sent on every slider value change via `DeviceEvent()`
- Continuous updates as user drags slider
- Initial value: 0
- Used by Living Room and Sofa screens for lamp control

**Example Notification Data:**
```
Hex: 0x02 0x32
Device ID: 0x02 (Lamp 1)
Value: 50
Meaning: Lamp 1 set to 50%

Hex: 0x03 0x64
Device ID: 0x03 (Lamp 2)
Value: 100
Meaning: Lamp 2 set to 100%
```

---

### 2. Button Event Characteristic

**UUID:** `00060200-78fc-48fe-8e23-433b3a1942d0`

**Properties:**
- `READ` - Can read (always returns 0)
- `NOTIFY` - Sends notifications when button is pressed

**Data Format:**
- Type: `uint8_t` (1 byte)
- Value: Device ID (0x01 for door control, etc.)
- Encoding: Little-endian

**Behavior:**
- Notification sent when button is clicked
- Single notification per button press
- No long-press detection
- Used by Door screen for door control

**Example Notification Data:**
```
Hex: 0x01
Device ID: 0x01
Meaning: Door button pressed
```

---

### 3. Status Characteristic

**UUID:** `00060300-78fc-48fe-8e23-433b3a1942d0`

**Properties:**
- `READ` - Can read current status
- `WRITE` - Can write status from client

**Data Format:**
- Type: String (ASCII)
- Max Length: Variable
- Encoding: UTF-8

**Behavior:**
- Client can write status messages to the watch
- Watch displays status in screen labels
- Default: Empty string

**Example Write Data:**
```
String: "Connected to server"
Hex: 43 6F 6E 6E 65 63 74 65 64 20 74 6F 20 73 65 72 76 65 72
```

---

## Connection Flow

### 1. Discovery

```python
# Scan for device
device_name = "LassiBLEHero"
device_address = "XX:XX:XX:XX:XX:XX"  # Find via scan

# Connect to device
# Use standard BLE connection methods for your platform
```

### 2. Service Discovery

```python
# Discover services
service_uuid = "00060000-78fc-48fe-8e23-433b3a1942d0"

# Discover characteristics
slider_uuid = "00060100-78fc-48fe-8e23-433b3a1942d0"
button_uuid = "00060200-78fc-48fe-8e23-433b3a1942d0"
status_uuid = "00060300-78fc-48fe-8e23-433b3a1942d0"
```

### 3. Enable Notifications

```python
# Enable notifications for slider events
enable_notification(slider_uuid)

# Enable notifications for button events
enable_notification(button_uuid)

# Register notification handlers
register_handler(slider_uuid, on_slider_change)
register_handler(button_uuid, on_button_press)
```

---

## Design Philosophy: Learning from Gadgetbridge

The RemoteControlService design is inspired by InfiniTime's MusicService protocol, which Gadgetbridge uses to send music playback information to the watch. However, our service works in the **opposite direction**:

**MusicService (Gadgetbridge → Watch):**
- Phone **writes** track info (artist, title, album) TO watch
- Watch **notifies** control events (play/pause/next) back to phone
- Uses the UUID pattern: `0000XXYY-78fc-48fe-8e23-433b3a1942d0`

**RemoteControlService (Watch → Phone):**
- Watch **notifies** UI events (button/slider) TO phone
- Phone can **write** status messages back to watch
- Uses the UUID pattern: `0006XXYY-78fc-48fe-8e23-433b3a1942d0`

This inverse design allows the watch to act as a remote control for external applications, rather than displaying information from them.

### Key Patterns from Gadgetbridge

#### 1. Change Detection Pattern

Gadgetbridge only sends updates when values change to reduce BLE traffic. We recommend the same pattern for your client:

```python
class SmartNotificationHandler:
    def __init__(self):
        self.last_slider_value = None

    def slider_notification_handler(self, sender, data):
        value = data[0]

        # Only process if value changed
        if value != self.last_slider_value:
            self.last_slider_value = value
            self.on_slider_change(value)  # Your processing logic
```

#### 2. Safe Writing Pattern

Always check if characteristics support the operations you need:

```python
async def safe_write(client, uuid, data):
    """Safely write to characteristic with error handling"""
    try:
        # Verify characteristic exists and supports writing
        services = await client.get_services()
        char = services.get_characteristic(uuid)

        if char and 'write' in char.properties:
            await client.write_gatt_char(uuid, data)
            return True
        else:
            print(f"Characteristic {uuid} doesn't support writing")
            return False
    except Exception as e:
        print(f"Write failed: {e}")
        return False
```

#### 3. String Truncation (for Status characteristic)

InfiniTime limits strings to 40 characters with ellipsis truncation:

```python
def truncate_status(message, max_length=40):
    """Truncate message like InfiniTime does"""
    if len(message) <= max_length:
        return message
    else:
        # Truncate with ellipsis (like InfiniTime's music service)
        return message[:max_length-3] + "..."

# Usage
status = "This is a very long status message that needs truncation"
await client.write_gatt_char(STATUS_UUID, truncate_status(status).encode('utf-8'))
```

---

## Python Implementation Guide

### Production-Ready Implementation (Bleak)

This implementation includes Gadgetbridge-inspired patterns:

```python
import asyncio
import struct
from bleak import BleakClient, BleakScanner

class LassiBLEServer:
    """
    Production-ready BLE client for LassiBLEHero watch.
    Implements patterns learned from Gadgetbridge's MusicService.
    """
    SERVICE_UUID = "00060000-78fc-48fe-8e23-433b3a1942d0"
    SLIDER_UUID = "00060100-78fc-48fe-8e23-433b3a1942d0"
    BUTTON_UUID = "00060200-78fc-48fe-8e23-433b3a1942d0"
    STATUS_UUID = "00060300-78fc-48fe-8e23-433b3a1942d0"

    MAX_STATUS_LENGTH = 40  # InfiniTime string limit

    def __init__(self, device_address):
        self.device_address = device_address
        self.client = None

        # Change detection (Gadgetbridge pattern)
        self.last_slider_value = None
        self.last_button_value = None

        # Callbacks
        self.on_slider_change = None
        self.on_button_press = None

    async def connect(self):
        """Connect to watch and verify service availability"""
        self.client = BleakClient(self.device_address)
        await self.client.connect()

        # Verify service exists
        services = await self.client.get_services()
        if not services.get_service(self.SERVICE_UUID):
            raise Exception("RemoteControlService not found on device")

        print(f"Connected to LassiBLEHero: {self.client.is_connected}")

    async def enable_notifications(self):
        """Enable notifications for slider and button events"""
        try:
            await self.client.start_notify(
                self.SLIDER_UUID,
                self._slider_notification_handler
            )
            print("Slider notifications enabled")
        except Exception as e:
            print(f"Failed to enable slider notifications: {e}")

        try:
            await self.client.start_notify(
                self.BUTTON_UUID,
                self._button_notification_handler
            )
            print("Button notifications enabled")
        except Exception as e:
            print(f"Failed to enable button notifications: {e}")

    def _slider_notification_handler(self, sender, data):
        """Internal handler for slider notifications with change detection"""
        if len(data) >= 2:
            device_id = data[0]
            value = data[1]  # 0-100
            
            # Device ID mapping
            device_names = {
                0x02: "Living Room Lamp 1",
                0x03: "Living Room Lamp 2",
                0x04: "Sofa Lamp 1",
                0x05: "Sofa Lamp 2"
            }
            
            device_name = device_names.get(device_id, f"Unknown Device 0x{device_id:02x}")
            print(f"{device_name}: {value}%")

            # Call user callback if registered
            if self.on_slider_change:
                self.on_slider_change(device_id, value)

    def _button_notification_handler(self, sender, data):
        """Internal handler for button notifications"""
        device_id = data[0]
        
        # Device ID mapping
        device_names = {
            0x01: "Door Control"
        }
        
        device_name = device_names.get(device_id, f"Unknown Button 0x{device_id:02x}")
        print(f"Button pressed: {device_name}")

        # Call user callback if registered
        if self.on_button_press:
            self.on_button_press(device_id)

    async def write_status(self, message):
        """
        Write status message to watch (like Gadgetbridge writes track info).
        Automatically truncates to 40 characters with ellipsis.
        """
        # Truncate with ellipsis if needed
        if len(message) > self.MAX_STATUS_LENGTH:
            truncated = message[:self.MAX_STATUS_LENGTH-3] + "..."
        else:
            truncated = message

        try:
            await self.client.write_gatt_char(
                self.STATUS_UUID,
                truncated.encode('utf-8')
            )
            return True
        except Exception as e:
            print(f"Failed to write status: {e}")
            return False

    async def read_slider_value(self):
        """Read current slider value from watch"""
        try:
            data = await self.client.read_gatt_char(self.SLIDER_UUID)
            return data[0]
        except Exception as e:
            print(f"Failed to read slider: {e}")
            return None

    async def disconnect(self):
        """Gracefully disconnect from watch"""
        if self.client and self.client.is_connected:
            await self.client.disconnect()
            print("Disconnected from LassiBLEHero")

# Example usage with callbacks
async def main():
    # Find device
    print("Scanning for LassiBLEHero...")
    devices = await BleakScanner.discover()

    lassi_device = None
    for d in devices:
        if d.name == "LassiBLEHero":
            lassi_device = d.address
            print(f"Found device at {lassi_device}")
            break

    if not lassi_device:
        print("Device not found!")
        return

    # Create client
    server = LassiBLEServer(lassi_device)

    # Register callbacks
    def handle_slider(device_id, value):
        print(f"Application received device {device_id:02x}: {value}%")
        # Your application logic here
        # Example: Control home automation lights
        if device_id == 0x02:
            # Control Living Room Lamp 1
            pass
        elif device_id == 0x03:
            # Control Living Room Lamp 2
            pass

    def handle_button(device_id):
        print(f"Application received button from device {device_id:02x}")
        # Your application logic here
        if device_id == 0x01:
            # Trigger door opening
            pass

    server.on_slider_change = handle_slider
    server.on_button_press = handle_button

    # Connect and setup
    await server.connect()
    await server.enable_notifications()

    # Send acknowledgment to watch
    await server.write_status("Connected!")

    # Keep connection alive and process events
    print("Listening for events... (Press Ctrl+C to exit)")
    try:
        while True:
            await asyncio.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        await server.disconnect()

# Run
if __name__ == "__main__":
    asyncio.run(main())
```

### Legacy BlueZ Implementation (Linux)

For systems using BlueZ directly:

```python
import dbus
from gi.repository import GLib

class LassiBLEClient:
    SERVICE_UUID = "00060000-78fc-48fe-8e23-433b3a1942d0"
    SLIDER_UUID = "00060100-78fc-48fe-8e23-433b3a1942d0"
    BUTTON_UUID = "00060200-78fc-48fe-8e23-433b3a1942d0"
    STATUS_UUID = "00060300-78fc-48fe-8e23-433b3a1942d0"

    def __init__(self, device_address):
        self.device_address = device_address
        self.bus = dbus.SystemBus()

    def connect(self):
        # Get device object
        device_path = f"/org/bluez/hci0/dev_{self.device_address.replace(':', '_')}"
        device = self.bus.get_object("org.bluez", device_path)
        device_interface = dbus.Interface(device, "org.bluez.Device1")

        # Connect
        device_interface.Connect()

    def discover_characteristics(self):
        # Find service and characteristics
        # Implementation depends on your BlueZ version
        pass

    def enable_notifications(self, char_uuid):
        # Enable notifications for characteristic
        char = self.get_characteristic(char_uuid)
        char.StartNotify()

    def on_slider_notification(self, interface, changed_props, invalidated_props):
        if "Value" in changed_props:
            value = changed_props["Value"][0]  # uint8_t
            print(f"Slider changed: {value}%")
            # Handle slider event

    def on_button_notification(self, interface, changed_props, invalidated_props):
        if "Value" in changed_props:
            value = changed_props["Value"][0]  # Always 5
            print("Button pressed!")
            # Handle button event

    def write_status(self, status_message):
        char = self.get_characteristic(self.STATUS_UUID)
        data = [dbus.Byte(c) for c in status_message.encode('utf-8')]
        char.WriteValue(data, {})
```

### Using Bleak (Cross-platform Python)

```python
import asyncio
from bleak import BleakClient, BleakScanner

class LassiBLEClient:
    SERVICE_UUID = "00060000-78fc-48fe-8e23-433b3a1942d0"
    SLIDER_UUID = "00060100-78fc-48fe-8e23-433b3a1942d0"
    BUTTON_UUID = "00060200-78fc-48fe-8e23-433b3a1942d0"
    STATUS_UUID = "00060300-78fc-48fe-8e23-433b3a1942d0"

    def __init__(self, device_address):
        self.device_address = device_address
        self.client = None

    async def connect(self):
        self.client = BleakClient(self.device_address)
        await self.client.connect()
        print(f"Connected: {self.client.is_connected}")

    async def enable_notifications(self):
        # Enable slider notifications
        await self.client.start_notify(
            self.SLIDER_UUID,
            self.slider_notification_handler
        )

        # Enable button notifications
        await self.client.start_notify(
            self.BUTTON_UUID,
            self.button_notification_handler
        )

    def slider_notification_handler(self, sender, data):
        """Handle device events (sliders with device IDs)"""
        if len(data) >= 2:
            device_id = data[0]
            value = data[1]
            print(f"Device 0x{device_id:02x}: {value}%")
            # Process device event here
            # Map device_id to specific controls in your application

    def button_notification_handler(self, sender, data):
        """Handle button presses with device IDs"""
        device_id = data[0]
        print(f"Button pressed! Device ID: 0x{device_id:02x}")
        # Process button event here

    async def write_status(self, message):
        """Write status message to watch"""
        await self.client.write_gatt_char(
            self.STATUS_UUID,
            message.encode('utf-8')
        )

    async def read_slider_value(self):
        """Read current slider value"""
        data = await self.client.read_gatt_char(self.SLIDER_UUID)
        return int.from_bytes(data, byteorder='little')

    async def disconnect(self):
        await self.client.disconnect()

# Example usage
async def main():
    # Scan for device
    devices = await BleakScanner.discover()
    lassi_device = None
    for d in devices:
        if d.name == "LassiBLEHero":
            lassi_device = d.address
            break

    if not lassi_device:
        print("Device not found!")
        return

    # Connect and setup
    client = LassiBLEClient(lassi_device)
    await client.connect()
    await client.enable_notifications()

    # Write status to watch
    await client.write_status("Connected!")

    # Keep connection alive
    await asyncio.sleep(60)

    # Cleanup
    await client.disconnect()

# Run
asyncio.run(main())
```

---

## Testing with bluez_test.py

A basic test script is provided in the repository:

```bash
# Prerequisites
pip install dbus-python pygobject

# Ensure watch is paired
bluetoothctl
> pair XX:XX:XX:XX:XX:XX
> trust XX:XX:XX:XX:XX:XX

# Run test
python3 bluez_test.py
```

The script will:
1. Connect to LassiBLEHero
2. Discover the RemoteControlService
3. Enable notifications for button/slider events
4. Print received events in hex format
5. Handle graceful disconnect on Ctrl+C

---

## Event Data Formats

### Device Event (via SliderEventHandle)

Used by `DeviceEvent()` method to send multi-byte device control data.

```
Byte 0: Device ID
Byte 1: Value (0-100)

Device ID Mapping:
0x01 = Door control (button)
0x02 = Living Room Lamp 1 (slider)
0x03 = Living Room Lamp 2 (slider)
0x04 = Sofa Lamp 1 (slider)
0x05 = Sofa Lamp 2 (slider)

Examples:
0x02 0x32 = Living Room Lamp 1 at 50%
0x03 0x64 = Living Room Lamp 2 at 100%
0x04 0x00 = Sofa Lamp 1 off (0%)
0x05 0x19 = Sofa Lamp 2 at 25%
```

### Button Event

Used by `ButtonEvent()` method for single-action triggers.

```
Byte 0: Device ID

Examples:
0x01 = Door button pressed
```

### Status Message

```
String: UTF-8 encoded text
No length prefix
Null-terminated on C side

Examples:
"ok" = 0x6F 0x6B
"Connected" = 0x43 0x6F 0x6E 0x6E 0x65 0x63 0x74 0x65 0x64
```

---

## MyApp User Interface

MyApp is a multi-screen application with swipeable pages for different room controls. Navigate between screens by swiping up/down.

### Screen 1: Door Control

**UI Elements:**
1. **Title Label** - "Door Control"
2. **Button** (center) - "OPEN"
   - Sends button event with device ID 0x01
   - Used to trigger door opening mechanism
3. **Status Label** (bottom) - Shows "Ready" or connection status

**BLE Events:**
- Button press → ButtonEvent with device ID 0x01

---

### Screen 2: Living Room

**UI Elements:**
1. **Title Label** - "Living Room"
2. **Lamp 1 Label** - Shows "Lamp 1: X" (current value)
3. **Slider 1** - Controls Lamp 1 brightness (0-100)
4. **Lamp 2 Label** - Shows "Lamp 2: X" (current value)
5. **Slider 2** - Controls Lamp 2 brightness (0-100)

**BLE Events:**
- Slider 1 change → DeviceEvent with [0x02, value]
- Slider 2 change → DeviceEvent with [0x03, value]

---

### Screen 3: Sofa Area

**UI Elements:**
1. **Title Label** - "Sofa"
2. **Lamp 1 Label** - Shows "Lamp 1: X" (current value)
3. **Slider 1** - Controls Lamp 1 brightness (0-100)
4. **Lamp 2 Label** - Shows "Lamp 2: X" (current value)
5. **Slider 2** - Controls Lamp 2 brightness (0-100)

**BLE Events:**
- Slider 1 change → DeviceEvent with [0x04, value]
- Slider 2 change → DeviceEvent with [0x05, value]

---

## Troubleshooting

### Device Not Found
- Ensure watch is awake and BLE is enabled
- Check device name is "LassiBLEHero"
- Try scanning multiple times
- Verify watch is not connected to another device

### Connection Failed
- Pair device first using `bluetoothctl`
- Check Bluetooth adapter is enabled
- Verify sufficient permissions (may need root/sudo on Linux)
- Restart Bluetooth service: `sudo systemctl restart bluetooth`

### No Notifications Received
- Ensure notifications are enabled for characteristics
- Check MyApp is running on the watch
- Verify proper notification handler registration
- Try reading characteristic first to test connection

### Notification Data Format Issues
- Remember: data is little-endian
- Slider: single byte (0-100)
- Button: single byte (always 5)
- Status: UTF-8 string

---

## Advanced Usage

### Custom Event Values

To add new device controls, assign a new device ID and use the appropriate event method:

**For button controls (single-byte events):**
```cpp
// In your screen's OnObjectEvent handler
const uint8_t deviceId = 0x06;  // New device ID
const char* msg = remoteControlService.ButtonEvent(&deviceId);
```

**For slider controls (two-byte events):**
```cpp
// In your screen's OnObjectEvent handler
uint8_t val = lv_slider_get_value(slider);
uint8_t data[2] = {0x07, val};  // Device ID 0x07, value 0-100
remoteControlService.DeviceEvent(data, 2);
```

**Example - Adding a new room:**
Edit `src/displayapp/screens/MyAppNewRoom.cpp`:
```cpp
void MyAppNewRoom::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_VALUE_CHANGED && obj == slider1) {
    uint8_t val = lv_slider_get_value(slider1);
    uint8_t data[2] = {0x08, val};  // Device ID 0x08
    remoteControlService.DeviceEvent(data, 2);
  }
}
```

### Adding More Characteristics

To add new event types, modify `src/components/ble/RemoteControlService.cpp`:

1. Define new UUID (follow pattern: `0006XXYY-78fc-48fe-8e23-433b3a1942d0`)
2. Add characteristic definition to constructor
3. Implement notification method similar to `ButtonEvent()`/`SliderEvent()`
4. Add READ handler in `OnCommand()` if needed

---

## Protocol Summary

**Connection:**
- Device advertises as "LassiBLEHero"
- Standard BLE GATT connection

**Service Discovery:**
- Service: `00060000-78fc-48fe-8e23-433b3a1942d0`
- 3 characteristics with UUIDs 00060100, 00060200, 00060300

**Data Flow:**
- Watch → Client: Notifications for slider/button events
- Client → Watch: Write status messages

**Timing:**
- Slider: Continuous updates during interaction
- Button: Single event per press
- No batching or queuing

---

## Requirements

**Watch Side:**
- LassiBLEHero firmware v1.15.0+
- MyApp running

**Client Side:**
- BLE 4.0+ compatible adapter
- Python 3.7+
- BlueZ (Linux) or platform-specific BLE stack
- Bleak library (recommended for cross-platform)

---

## Comparison: RemoteControlService vs MusicService

Understanding the differences helps when adapting Gadgetbridge patterns:

| Feature | MusicService (00000000-...) | RemoteControlService (00060000-...) |
|---------|----------------------------|-------------------------------------|
| **Direction** | Phone → Watch (display info) | Watch → Phone (send events) |
| **Primary Chars** | Artist, Track, Album (WRITE) | Slider, Button (NOTIFY) |
| **Data Types** | Strings (UTF-8), Integers (BE) | Single bytes (little-endian) |
| **Use Case** | Display music metadata | Remote control input |
| **Update Pattern** | On track change | On user interaction |
| **String Limit** | 40 chars with ellipsis | 40 chars (Status char only) |
| **Control Flow** | Phone controls watch display | Watch controls phone app |
| **Events** | Watch sends play/pause TO phone | Watch sends button/slider TO phone |

### Encoding Differences

**MusicService:**
```python
# Strings: UTF-8
artist = "Artist Name".encode('utf-8')

# Integers: Big-endian (network byte order)
position = struct.pack('>I', 120)  # 0x00 0x00 0x00 0x78
```

**RemoteControlService:**
```python
# Simple bytes: Little-endian (native)
slider = bytes([50])  # Just 0x32

# Strings: UTF-8 (Status char only)
status = "Connected".encode('utf-8')
```

### Pattern Similarities

Both services use:
- ✅ Same UUID pattern (`xxxxxxxx-78fc-48fe-8e23-433b3a1942d0`)
- ✅ Change detection to reduce BLE traffic
- ✅ Safe writing with error handling
- ✅ 40-character string truncation
- ✅ NOTIFY characteristics for real-time updates

### When to Use Each Pattern

**Use MusicService patterns when:**
- Sending complex data structures to watch
- Displaying information from phone/server
- Need big-endian integer encoding (for compatibility)
- Writing multiple related values (artist, track, album)

**Use RemoteControlService patterns when:**
- Receiving simple events from watch
- Watch acts as input device
- Single-byte values are sufficient
- Real-time interaction feedback needed

---

## InfiniTime BLE Services Overview

For context, here are all InfiniTime's custom BLE services:

| Service | UUID Base | Purpose | Direction |
|---------|-----------|---------|-----------|
| Music | `00000000-...` | Music playback control | Phone ↔ Watch |
| Navigation | `00010000-...` | Navigation instructions | Phone → Watch |
| **Remote Control** | `00060000-...` | UI event notifications | **Watch → Phone** |
| Weather | Custom | Weather information | Phone → Watch |
| Alert Notification | Standard | Notifications | Phone → Watch |
| Current Time | Standard | Time sync | Phone → Watch |

Your RemoteControlService fills a unique niche: it's the only service designed primarily for **watch-to-phone event streaming**.

---

## References

- [InfiniTime Documentation](https://github.com/InfiniTimeOrg/InfiniTime)
- [InfiniTime BLE Protocol Docs](https://github.com/InfiniTimeOrg/InfiniTime/blob/main/doc/ble.md)
- [Gadgetbridge Repository](https://codeberg.org/Freeyourgadget/Gadgetbridge)
- [Gadgetbridge PineTime Support](https://github.com/Freeyourgadget/Gadgetbridge/blob/master/app/src/main/java/nodomain/freeyourgadget/gadgetbridge/service/devices/pinetime/PineTimeJFSupport.java)
- [NimBLE Documentation](https://github.com/apache/mynewt-nimble)
- [Bleak Python BLE Library](https://github.com/hbldh/bleak)
- [BlueZ GATT API](https://github.com/bluez/bluez/blob/master/doc/gatt-api.txt)

---

## License

This project is based on InfiniTime, which is licensed under GNU GPL v3.

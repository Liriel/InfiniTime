# LassiBLEHero - BLE Remote Control Documentation

## Overview

LassiBLEHero is a custom InfiniTime firmware for PineTime that adds a BLE Remote Control Service. This service enables the watch to send UI events (button presses and slider changes) to connected BLE clients in real-time.

**Device Name:** `LassiBLEHero`

**Firmware Version:** 1.15.0 (based on InfiniTime)

**Custom Features:**
- BLE Remote Control Service for sending watch UI events
- MyApp - Demo application with interactive button and slider
- Terminal watchface with "s7arbuck" branding

---

## BLE Service Architecture

### Remote Control Service

**Service UUID:** `00060000-78fc-48fe-8e23-433b3a1942d0`

**Service Type:** Primary GATT Service

**Purpose:** Sends notifications when the user interacts with MyApp's UI elements (button clicks and slider movements).

---

## Characteristics

### 1. Slider Event Characteristic

**UUID:** `00060100-78fc-48fe-8e23-433b3a1942d0`

**Properties:**
- `READ` - Can read current slider value
- `NOTIFY` - Sends notifications when slider value changes

**Data Format:**
- Type: `uint8_t` (1 byte)
- Range: `0-100`
- Encoding: Little-endian

**Behavior:**
- Notification sent on every slider value change
- Continuous updates as user drags slider
- Initial value: 0

**Example Notification Data:**
```
Hex: 0x32
Decimal: 50
Meaning: Slider at 50%
```

---

### 2. Button Event Characteristic

**UUID:** `00060200-78fc-48fe-8e23-433b3a1942d0`

**Properties:**
- `READ` - Can read (always returns 0)
- `NOTIFY` - Sends notifications when button is pressed

**Data Format:**
- Type: `uint8_t` (1 byte)
- Value: Always `0x05` (5)
- Encoding: Little-endian

**Behavior:**
- Notification sent when button is clicked
- Single notification per button press
- No long-press detection

**Example Notification Data:**
```
Hex: 0x05
Decimal: 5
Meaning: Button pressed
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
- Watch displays status in MyApp title when button is pressed
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

## Python Implementation Guide

### Using BlueZ (Linux)

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
        """Handle slider value changes"""
        value = int.from_bytes(data, byteorder='little')
        print(f"Slider: {value}%")
        # Process slider event here

    def button_notification_handler(self, sender, data):
        """Handle button presses"""
        value = int.from_bytes(data, byteorder='little')
        print(f"Button pressed! (value={value})")
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

### Slider Event

```
Byte 0: Slider value (0-100)

Examples:
0x00 = 0% (minimum)
0x32 = 50% (middle)
0x64 = 100% (maximum)
```

### Button Event

```
Byte 0: Event code (always 0x05)

Example:
0x05 = Button pressed
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

The MyApp provides the following UI elements:

1. **Title Label** (top-left)
   - Shows connection status or slider value
   - Updates when button is pressed or slider moves

2. **Button** (center)
   - Label: "Send!"
   - Sends button event (value=5) via BLE
   - Displays response status in title

3. **Slider** (bottom-center)
   - Range: 0-100
   - Sends slider value via BLE on every change
   - Real-time updates during drag

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

The button currently sends a fixed value (5). To modify:

Edit `src/displayapp/screens/MyApp.cpp`:
```cpp
void MyApp::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED && obj == btnVolDown) {
    const uint8_t val = 5;  // Change this value
    const char* msg = remoteControlService.ButtonEvent(&val);
    lv_label_set_text(title, msg);
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

## References

- [InfiniTime Documentation](https://github.com/InfiniTimeOrg/InfiniTime)
- [NimBLE Documentation](https://github.com/apache/mynewt-nimble)
- [Bleak Python BLE Library](https://github.com/hbldh/bleak)
- [BlueZ GATT API](https://github.com/bluez/bluez/blob/master/doc/gatt-api.txt)

---

## License

This project is based on InfiniTime, which is licensed under GNU GPL v3.

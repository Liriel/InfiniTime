#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

import dbus
try:
  from gi.repository import GObject
except ImportError:
  import gobject as GObject
import sys
import signal

from dbus.mainloop.glib import DBusGMainLoop

bus = None
mainloop = None
dev = None

BLUEZ_SERVICE_NAME = 'org.bluez'
DBUS_OM_IFACE =      'org.freedesktop.DBus.ObjectManager'
DBUS_PROP_IFACE =    'org.freedesktop.DBus.Properties'

GATT_SERVICE_IFACE = 'org.bluez.GattService1'
GATT_CHRC_IFACE =    'org.bluez.GattCharacteristic1'

SVC_UUID = '00060000-78fc-48fe-8e23-433b3a1942d0'
BTN_UUID = '00060200-78fc-48fe-8e23-433b3a1942d0'
SLD_UUID = '00060100-78fc-48fe-8e23-433b3a1942d0'

# The objects that we interact with.
hr_service = None
btn_chrc = None
sld_chrc = None

# Device names for pretty printing
DEVICE_NAMES = {
    0x01: "Door",
    0x02: "Living Room Lamp 1",
    0x03: "Living Room Lamp 2",
    0x04: "Sofa Lamp 1",
    0x05: "Sofa Lamp 2"
}

# define CTRL+C signal handler
def signal_handler(signal, frame):
    print("CTRL + C - terminating")
    if(dev):
       print("disco")
       dev.Disconnect()
    sys.exit(0)


def generic_error_cb(error):
    print('D-Bus call failed: ' + str(error))
    mainloop.quit()



def sensor_contact_val_to_str(val):
    if val == 0 or val == 1:
        return 'not supported'
    if val == 2:
        return 'no contact detected'
    if val == 3:
        return 'contact detected'

    return 'invalid value'



def hr_msrmt_start_notify_cb():
    print('Notifications enabled')


def hr_msrmt_changed_cb(iface, changed_props, invalidated_props):
    if iface != GATT_CHRC_IFACE:
        return

    if not len(changed_props):
        return

    value = changed_props.get('Value', None)
    if not value:
        return

    # Parse 2-byte format: [device_id, value]
    if len(value) >= 2:
        device_id = value[0]
        device_value = value[1]
        device_name = DEVICE_NAMES.get(device_id, f"Unknown Device 0x{device_id:02x}")
        print(f"{device_name}: {device_value}")
    elif len(value) == 1:
        # Legacy single-byte format
        print(f"Value: {value[0]}")
    else:
        print('Notification received:')
        for v in value:
            print("  %02x" % v)


def start_client():

    # Listen to PropertiesChanged signals from both characteristics
    if btn_chrc:
        btn_prop_iface = dbus.Interface(btn_chrc[0], DBUS_PROP_IFACE)
        btn_prop_iface.connect_to_signal("PropertiesChanged", hr_msrmt_changed_cb)
        btn_chrc[0].StartNotify(reply_handler=hr_msrmt_start_notify_cb,
                                error_handler=generic_error_cb,
                                dbus_interface=GATT_CHRC_IFACE)
        print("Subscribed to Button Events")

    if sld_chrc:
        sld_prop_iface = dbus.Interface(sld_chrc[0], DBUS_PROP_IFACE)
        sld_prop_iface.connect_to_signal("PropertiesChanged", hr_msrmt_changed_cb)
        sld_chrc[0].StartNotify(reply_handler=hr_msrmt_start_notify_cb,
                                error_handler=generic_error_cb,
                                dbus_interface=GATT_CHRC_IFACE)
        print("Subscribed to Slider Events")


def process_chrc(chrc_path):
    chrc = bus.get_object(BLUEZ_SERVICE_NAME, chrc_path)
    chrc_props = chrc.GetAll(GATT_CHRC_IFACE,
                             dbus_interface=DBUS_PROP_IFACE)

    uuid = chrc_props['UUID']

    if uuid == BTN_UUID:
        global btn_chrc
        btn_chrc = (chrc, chrc_props)
        print('Button Characteristic found: ' + uuid)
    elif uuid == SLD_UUID:
        global sld_chrc
        sld_chrc = (chrc, chrc_props)
        print('Slider Characteristic found: ' + uuid)

    return True


def process_hr_service(service_path, chrc_paths):
    service = bus.get_object(BLUEZ_SERVICE_NAME, service_path)
    service_props = service.GetAll(GATT_SERVICE_IFACE,
                                   dbus_interface=DBUS_PROP_IFACE)

    uuid = service_props['UUID']

    if uuid != SVC_UUID:
        return False

    print('Service found: ' + service_path)

    # Process the characteristics.
    for chrc_path in chrc_paths:
        process_chrc(chrc_path)

    global hr_service
    hr_service = (service, service_props, service_path)

    return True


def interfaces_removed_cb(object_path, interfaces):
    if not hr_service:
        return

    if object_path == hr_service[2]:
        print('Service was removed')
        mainloop.quit()


def main():
    # register CTRL+C signal handler
    signal.signal(signal.SIGINT, signal_handler)

    # Set up the main loop.
    DBusGMainLoop(set_as_default=True)
    global bus
    bus = dbus.SystemBus()
    global mainloop
    mainloop = GObject.MainLoop()

    om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/'), DBUS_OM_IFACE)
    om.connect_to_signal('InterfacesRemoved', interfaces_removed_cb)

    print('Getting objects...')
    objects = om.GetManagedObjects()
    chrcs = []

    # List characteristics found
    for path, interfaces in objects.items():
        if GATT_CHRC_IFACE not in interfaces.keys():
            continue
        chrcs.append(path)

    # List sevices found
    for path, interfaces in objects.items():
        if GATT_SERVICE_IFACE not in interfaces.keys():
            continue

        chrc_paths = [d for d in chrcs if d.startswith(path + "/")]

        if process_hr_service(path, chrc_paths):
            break

    if not hr_service:
        print('No Remote Control Service found')
        sys.exit(1)

    if not btn_chrc and not sld_chrc:
        print('No characteristics found')
        sys.exit(1)

    global dev 
    dev = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, hr_service[1]['Device']), "org.bluez.Device1")
    dev.Connect()
    start_client()

    mainloop.run()


if __name__ == '__main__':
    main()

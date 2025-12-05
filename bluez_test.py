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

SVC_UUID = '00000000-78fc-48fe-8e23-433b3a1942d0'
BTN_UUID = '00000001-78fc-48fe-8e23-433b3a1942d0'

# The objects that we interact with.
hr_service = None
btn_chrc = None

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

    print('ping')
    for v in value:
        print("%x" % v)


def start_client():

    # Listen to PropertiesChanged signals from the Heart Measurement
    # Characteristic.
    hr_msrmt_prop_iface = dbus.Interface(btn_chrc[0], DBUS_PROP_IFACE)
    hr_msrmt_prop_iface.connect_to_signal("PropertiesChanged",
                                         hr_msrmt_changed_cb)

    # Subscribe to Heart Rate Measurement notifications.
    btn_chrc[0].StartNotify(reply_handler=hr_msrmt_start_notify_cb,
                                 error_handler=generic_error_cb,
                                 dbus_interface=GATT_CHRC_IFACE)


def process_chrc(chrc_path):
    chrc = bus.get_object(BLUEZ_SERVICE_NAME, chrc_path)
    chrc_props = chrc.GetAll(GATT_CHRC_IFACE,
                             dbus_interface=DBUS_PROP_IFACE)

    uuid = chrc_props['UUID']

    if uuid == BTN_UUID:
        global btn_chrc
        btn_chrc = (chrc, chrc_props)
        print('Characteristic found: ' + uuid)

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
        print('No Heart Rate Service found')
        sys.exit(1)

    global dev 
    dev = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, hr_service[1]['Device']), "org.bluez.Device1")
    dev.Connect()
    start_client()

    mainloop.run()


if __name__ == '__main__':
    main()

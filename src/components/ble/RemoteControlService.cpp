/*  Copyright (C) 2020-2021 JF, Adam Pigg, Avamander

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "components/ble/RemoteControlService.h"
#include "systemtask/SystemTask.h"

namespace {
  // 0006yyxx-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t CharUuid(uint8_t x, uint8_t y) {
    return ble_uuid128_t{
      .u = {.type = BLE_UUID_TYPE_128},
      .value =  { 0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, x, y, 0x06, 0x00 }
    };
  }

  // 00060000-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t BaseUuid() {
    return CharUuid(0x00, 0x00);
  }

  constexpr ble_uuid128_t msUuid {BaseUuid()};

  constexpr ble_uuid128_t msSliderEventCharUuid {CharUuid(0x00, 0x01)};
  constexpr ble_uuid128_t msButtonEventCharUuid {CharUuid(0x00, 0x02)};
  constexpr ble_uuid128_t msStatusCharUuid {CharUuid(0x00, 0x03)};

  int RemoteControlCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    return static_cast<Pinetime::Controllers::RemoteControlService*>(arg)->OnCommand(conn_handle, attr_handle, ctxt);
  }
}

Pinetime::Controllers::RemoteControlService::RemoteControlService(Pinetime::System::SystemTask& system) : m_system(system) {
  characteristicDefinition[0] = {.uuid = &msSliderEventCharUuid.u,
                                 .access_cb = RemoteControlCallback,
                                 .arg = this,
                                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                                 .val_handle = &sliderEventHandle};
  characteristicDefinition[1] = {.uuid = &msButtonEventCharUuid.u,
                                 .access_cb = RemoteControlCallback,
                                 .arg = this,
                                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                                 .val_handle = &buttonEventHandle};
  characteristicDefinition[2] = {.uuid = &msStatusCharUuid.u,
                                 .access_cb = RemoteControlCallback,
                                 .arg = this,
                                 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ};
  characteristicDefinition[3] = {0};

  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &msUuid.u, .characteristics = characteristicDefinition};
  serviceDefinition[1] = {0};
}

void Pinetime::Controllers::RemoteControlService::Init() {
  uint8_t res = 0;
  res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);

  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
}

int Pinetime::Controllers::RemoteControlService::OnCommand(uint16_t /*conn_handle*/, uint16_t /*attr_handle*/, struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    size_t notifSize = OS_MBUF_PKTLEN(ctxt->om);
    char data[notifSize + 1];
    data[notifSize] = '\0';
    os_mbuf_copydata(ctxt->om, 0, notifSize, data);
    char* s = &data[0];
    if (ble_uuid_cmp(ctxt->chr->uuid, &msStatusCharUuid.u) == 0) {
      status = s;
    }
  } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    // Handle READ operations for notify characteristics
    if (ble_uuid_cmp(ctxt->chr->uuid, &msSliderEventCharUuid.u) == 0) {
      uint8_t val = static_cast<uint8_t>(sliderValue);
      int res = os_mbuf_append(ctxt->om, &val, 1);
      return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    } else if (ble_uuid_cmp(ctxt->chr->uuid, &msButtonEventCharUuid.u) == 0) {
      uint8_t val = 0; // No stored button state
      int res = os_mbuf_append(ctxt->om, &val, 1);
      return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    } else if (ble_uuid_cmp(ctxt->chr->uuid, &msStatusCharUuid.u) == 0) {
      int res = os_mbuf_append(ctxt->om, status.c_str(), status.length());
      return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
  }
  return 0;
}

std::string Pinetime::Controllers::RemoteControlService::getStatus() const {
  return status;
}

int Pinetime::Controllers::RemoteControlService::getSliderValue() const {
  return sliderValue;
}

const char* Pinetime::Controllers::RemoteControlService::ButtonEvent(const uint8_t* event) {
  //uint8_t buffer[1] = { event };
  //const uint8_t d = 5;
  //auto* om = ble_hs_mbuf_from_flat(event, 1);
  auto* om = ble_hs_mbuf_from_flat(event, 1);

  uint16_t connectionHandle = m_system.nimble().connHandle();
  //m_system.nimble().connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return "no conn";
  }

  ble_gattc_notify_custom(connectionHandle, buttonEventHandle, om);
  return "done";
}

const char* Pinetime::Controllers::RemoteControlService::SliderEvent(const uint8_t* event) {
  //uint8_t buffer[1] = { event };
  //const uint8_t d = 5;
  //auto* om = ble_hs_mbuf_from_flat(event, 1);
  auto* om = ble_hs_mbuf_from_flat(event, 1);

  uint16_t connectionHandle = m_system.nimble().connHandle();
  //m_system.nimble().connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return "no conn";
  }

  ble_gattc_notify_custom(connectionHandle, sliderEventHandle, om);
  return "done";
}

const char* Pinetime::Controllers::RemoteControlService::DeviceEvent(const uint8_t* data, size_t len) {
  auto* om = ble_hs_mbuf_from_flat(data, len);

  uint16_t connectionHandle = m_system.nimble().connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return "no conn";
  }

  ble_gattc_notify_custom(connectionHandle, sliderEventHandle, om);
  return "done";
}

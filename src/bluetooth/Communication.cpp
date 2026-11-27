//
// Created by Михайло Грошевий on 21/01/2025.
//

#include "Communication.h"

#include <algorithm>

#include <pico/cyw43_arch.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "ble_config.h"

#define PARAMETERS_HANDLE        ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define TELEMETRY_HANDLE         ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define TELEMETRY_CONFIG_HANDLE  ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define COMMAND_HANDLE           ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE

#define TELEMETRY_INTERVAL_MS    25

constexpr uint8_t ADVERTISING_DATA[] = {
    0x02, 0x01, 0x06, // Length: 2 bytes, type: Bluetooth Data Type Flags (0x01), value: LE General Discoverable Mode + BR/EDR Not Supported (0x02 + 0x04)
    0x09, 0x09, 'T', 'r', 'i', 'a', 'n', 'g', 'l', 'e', // Length: 9 bytes, type: Complete Local Name (0x09), value: Triangle
    0x11, 0x07, // Length: 17 bytes, type: Complete List Of 128 Bit Service Class UUIDs (0x07)
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x10, 0xff, 0x00, 0x00  // value: 0000FF10-0000-1000-8000-00805F9B34FB
};

namespace {
    Communication* instance = nullptr;

    uint16_t attReadCallback(hci_con_handle_t, uint16_t attHandle, uint16_t offset, uint8_t* buffer,
                             uint16_t bufferSize) {
        return instance->onAttRead(attHandle, offset, buffer, bufferSize);
    }

    int attWriteCallback(hci_con_handle_t connection, uint16_t attHandle, uint16_t transactionMode, uint16_t offset,
                         uint8_t* buffer, uint16_t bufferSize) {
        if (transactionMode != ATT_TRANSACTION_MODE_NONE || offset != 0) {
            return 0;
        }
        return instance->onAttWrite(connection, attHandle, buffer, bufferSize);
    }

    void attEventCallback(uint8_t packetType, uint16_t, uint8_t* packet, uint16_t) {
        if (packetType == HCI_EVENT_PACKET && hci_event_packet_get_type(packet) == ATT_EVENT_DISCONNECTED) {
            instance->onDisconnected();
        }
    }

    void telemetryTimerCallback(btstack_timer_source_t*) {
        instance->onTelemetryTimer();
    }

    void canSendNowCallback(void*) {
        instance->onCanSendNow();
    }
}

Communication::Communication(Delegate& delegate, Telemetry& telemetry) : delegate(delegate), telemetry(telemetry) {
    hard_assert(instance == nullptr);
    instance = this;

    cyw43_arch_init();
    l2cap_init();
    sm_init();

    att_server_init(profile_data, attReadCallback, attWriteCallback);
    att_server_register_packet_handler(attEventCallback);
    canSendNowRegistration.callback = canSendNowCallback;

    btstack_run_loop_set_timer_handler(&telemetryTimer, telemetryTimerCallback);
    btstack_run_loop_set_timer(&telemetryTimer, TELEMETRY_INTERVAL_MS);
    btstack_run_loop_add_timer(&telemetryTimer);

    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, {}, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(ADVERTISING_DATA), (uint8_t*) ADVERTISING_DATA);
    gap_advertisements_enable(1);

    hci_power_control(HCI_POWER_ON);
}

int Communication::onAttRead(int attHandle, int offset, uint8_t* buffer, int bufferSize) {
    if (attHandle != PARAMETERS_HANDLE) {
        return 0;
    }

    triangle_v1_Parameters parameters = delegate.getParameters();
    uint8_t encoded[triangle_v1_Parameters_size];
    pb_ostream_t stream = pb_ostream_from_buffer(encoded, sizeof(encoded));
    if (!pb_encode(&stream, triangle_v1_Parameters_fields, &parameters)) {
        return 0;
    }
    return att_read_callback_handle_blob(encoded, stream.bytes_written, offset, buffer, bufferSize);
}

int Communication::onAttWrite(hci_con_handle_t connection, int attHandle, uint8_t* buffer, int bufferSize) {
    pb_istream_t stream = pb_istream_from_buffer(buffer, bufferSize);

    switch (attHandle) {
        case TELEMETRY_CONFIG_HANDLE:
            telemetryConnection =
                little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION
                    ? connection
                    : HCI_CON_HANDLE_INVALID;
            telemetry.clear();
            return 0;

        case PARAMETERS_HANDLE: {
            triangle_v1_Parameters parameters = triangle_v1_Parameters_init_zero;
            if (!pb_decode(&stream, triangle_v1_Parameters_fields, &parameters)) {
                return triangle_v1_Error_ERROR_INVALID_VALUE;
            }
            return delegate.setParameters(parameters) ? triangle_v1_Error_ERROR_NONE : triangle_v1_Error_ERROR_INVALID_VALUE;
        }

        case COMMAND_HANDLE: {
            triangle_v1_Command command = triangle_v1_Command_init_zero;
            if (!pb_decode(&stream, triangle_v1_Command_fields, &command)) {
                return triangle_v1_Error_ERROR_INVALID_VALUE;
            }
            if (command.type != triangle_v1_CommandType_COMMAND_TYPE_SAVE_PARAMETERS) {
                return triangle_v1_Error_ERROR_UNKNOWN_COMMAND;
            }
            return delegate.saveParameters() ? triangle_v1_Error_ERROR_NONE : triangle_v1_Error_ERROR_BUSY;
        }

        default:
            return 0;
    }
}

void Communication::onDisconnected() {
    telemetryConnection = HCI_CON_HANDLE_INVALID;
}

void Communication::onTelemetryTimer() {
    if (telemetryConnection != HCI_CON_HANDLE_INVALID && telemetry.available() > 0) {
        att_server_request_to_send_notification(&canSendNowRegistration, telemetryConnection);
    }

    btstack_run_loop_set_timer(&telemetryTimer, TELEMETRY_INTERVAL_MS);
    btstack_run_loop_add_timer(&telemetryTimer);
}

void Communication::onCanSendNow() {
    if (telemetryConnection == HCI_CON_HANDLE_INVALID) {
        return;
    }

    int payloadSize = att_server_get_mtu(telemetryConnection) - 3;
    int sampleCount = 0;
    int size = telemetry.encodeBatch(telemetryPacket, std::min(payloadSize, (int) sizeof(telemetryPacket)), sampleCount);
    if (size == 0) {
        return;
    }
    att_server_notify(telemetryConnection, TELEMETRY_HANDLE, telemetryPacket, size);

    if (telemetry.available() >= sampleCount) {
        att_server_request_to_send_notification(&canSendNowRegistration, telemetryConnection);
    }
}

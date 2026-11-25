//
// Created by Михайло Грошевий on 21/01/2025.
//

#include "Communication.h"

#include <algorithm>

#include <pico/cyw43_arch.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "ble_config.h"

#include "Telemetry.h"

#define PARAMETERS_HANDLE        ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define TELEMETRY_HANDLE         ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define TELEMETRY_CONFIG_HANDLE  ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define COMMAND_HANDLE           ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE

// Samples are batched: the timer only asks to send when there is data, and each notification
// carries as many samples as the negotiated MTU allows.
#define TELEMETRY_INTERVAL_MS    25

constexpr uint8_t advData[] = {
    0x02, 0x01, 0x06, // Length: 2 bytes, type: Bluetooth Data Type Flags (0x01), value: LE General Discoverable Mode + BR/EDR Not Supported (0x02 + 0x04)
    0x09, 0x09, 'T', 'r', 'i', 'a', 'n', 'g', 'l', 'e', // Length: 9 bytes, type: Complete Local Name (0x09), value: Triangle
    0x11, 0x07, // Length: 17 bytes, type: Complete List Of 128 Bit Service Class UUIDs (0x07)
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x10, 0xff, 0x00, 0x00  // value: 0000FF10-0000-1000-8000-00805F9B34FB
};

void Communication::init(const Handlers& newHandlers) {
    handlers = newHandlers;

    cyw43_arch_init();
    l2cap_init();
    sm_init();

    att_server_init(profile_data, attReadCallback, attWriteCallback);
    att_server_register_packet_handler(packetHandler);

    hciEventRegistration.callback = packetHandler;
    hci_add_event_handler(&hciEventRegistration);

    btstack_run_loop_set_timer_handler(&telemetryTimer, telemetryTimerHandler);
    btstack_run_loop_set_timer(&telemetryTimer, TELEMETRY_INTERVAL_MS);
    btstack_run_loop_add_timer(&telemetryTimer);

    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, {}, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(advData), (uint8_t*) advData);
    gap_advertisements_enable(1);

    hci_power_control(HCI_POWER_ON);
}

uint16_t Communication::attReadCallback(hci_con_handle_t connection, uint16_t attHandle, uint16_t offset,
                                        uint8_t* buffer, uint16_t bufferSize) {
    UNUSED(connection);

    if (attHandle == PARAMETERS_HANDLE) {
        triangle_v1_Parameters parameters = handlers.readParameters();
        uint8_t encoded[triangle_v1_Parameters_size];
        pb_ostream_t stream = pb_ostream_from_buffer(encoded, sizeof(encoded));
        if (!pb_encode(&stream, triangle_v1_Parameters_fields, &parameters)) {
            return 0;
        }
        return att_read_callback_handle_blob(encoded, stream.bytes_written, offset, buffer, bufferSize);
    }
    return 0;
}

int Communication::attWriteCallback(hci_con_handle_t connection, uint16_t attHandle, uint16_t transactionMode,
                                    uint16_t offset, uint8_t* buffer, uint16_t bufferSize) {
    if (transactionMode != ATT_TRANSACTION_MODE_NONE || offset != 0) {
        return 0;
    }

    switch (attHandle) {
        case TELEMETRY_CONFIG_HANDLE:
            connectionHandle = connection;
            isTelemetryEnabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
            Telemetry::clear();
            return 0;

        case PARAMETERS_HANDLE: {
            triangle_v1_Parameters parameters = triangle_v1_Parameters_init_zero;
            pb_istream_t stream = pb_istream_from_buffer(buffer, bufferSize);
            if (!pb_decode(&stream, triangle_v1_Parameters_fields, &parameters)) {
                return triangle_v1_Error_ERROR_INVALID_VALUE;
            }
            return handlers.writeParameters(parameters);
        }

        case COMMAND_HANDLE: {
            triangle_v1_Command command = triangle_v1_Command_init_zero;
            pb_istream_t stream = pb_istream_from_buffer(buffer, bufferSize);
            if (!pb_decode(&stream, triangle_v1_Command_fields, &command)) {
                return triangle_v1_Error_ERROR_INVALID_VALUE;
            }
            return handlers.executeCommand(command);
        }

        default:
            return 0;
    }
}

void Communication::packetHandler(uint8_t packetType, uint16_t channel, uint8_t* packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packetType != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            isTelemetryEnabled = false;
            connectionHandle = HCI_CON_HANDLE_INVALID;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            sendTelemetry();
            break;
        default:
            break;
    }
}

void Communication::telemetryTimerHandler(btstack_timer_source_t* timer) {
    if (isTelemetryEnabled) {
        if (Telemetry::available() > 0) {
            att_server_request_can_send_now_event(connectionHandle);
        }
    } else {
        Telemetry::clear();
    }

    btstack_run_loop_set_timer(timer, TELEMETRY_INTERVAL_MS);
    btstack_run_loop_add_timer(timer);
}

void Communication::sendTelemetry() {
    if (!isTelemetryEnabled) {
        return;
    }

    static uint8_t packet[triangle_v1_TelemetryBatch_size];
    size_t payloadSize = att_server_get_mtu(connectionHandle) - 3;
    size_t sampleCount = 0;
    size_t size = Telemetry::encodeBatch(packet, std::min(payloadSize, sizeof(packet)), sampleCount);
    if (size == 0) {
        return;
    }
    att_server_notify(connectionHandle, TELEMETRY_HANDLE, packet, size);

    if (Telemetry::available() >= sampleCount) {
        att_server_request_can_send_now_event(connectionHandle);
    }
}

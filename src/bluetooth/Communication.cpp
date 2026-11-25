//
// Created by Михайло Грошевий on 21/01/2025.
//

#include "Communication.h"

#include <algorithm>
#include <cstring>

#include <pico/cyw43_arch.h>
#include "ble_config.h"

#include "Telemetry.h"

#define PARAMETERS_HANDLE        ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define TELEMETRY_HANDLE         ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define TELEMETRY_CONFIG_HANDLE  ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define COMMAND_HANDLE           ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE

// Samples are batched: the timer only asks to send when there is data, and each notification
// carries as many samples as the negotiated MTU allows.
#define TELEMETRY_INTERVAL_MS    25
#define MAX_SAMPLES_PER_PACKET   18

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
        Protocol::Parameters parameters = handlers.readParameters();
        return att_read_callback_handle_blob((const uint8_t*) &parameters, sizeof(parameters), offset, buffer, bufferSize);
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
            Telemetry::clear(); // start the stream fresh instead of with stale samples
            return 0;

        case PARAMETERS_HANDLE: {
            if (bufferSize != sizeof(Protocol::Parameters)) {
                return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
            }
            Protocol::Parameters parameters;
            memcpy(&parameters, buffer, sizeof(parameters));
            return handlers.writeParameters(parameters);
        }

        case COMMAND_HANDLE:
            if (bufferSize != 1) {
                return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
            }
            return handlers.executeCommand((Protocol::Command) buffer[0]);

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

    static uint8_t packet[sizeof(Protocol::TelemetryHeader) + MAX_SAMPLES_PER_PACKET * sizeof(Protocol::TelemetrySample)];

    uint16_t payloadSize = att_server_get_mtu(connectionHandle) - 3;
    size_t maxSamples = std::min<size_t>(
        (payloadSize - sizeof(Protocol::TelemetryHeader)) / sizeof(Protocol::TelemetrySample), MAX_SAMPLES_PER_PACKET);

    auto* samples = reinterpret_cast<Protocol::TelemetrySample*>(packet + sizeof(Protocol::TelemetryHeader));
    uint16_t sequence = 0;
    size_t count = Telemetry::read(samples, maxSamples, sequence);
    if (count == 0) {
        return;
    }

    Protocol::TelemetryHeader header = {Protocol::TELEMETRY_VERSION, (uint8_t) count, sequence};
    memcpy(packet, &header, sizeof(header));
    att_server_notify(connectionHandle, TELEMETRY_HANDLE, packet,
                      sizeof(Protocol::TelemetryHeader) + count * sizeof(Protocol::TelemetrySample));

    // Keep going while a full packet is waiting; otherwise let the timer batch up the next one.
    if (Telemetry::available() >= maxSamples) {
        att_server_request_can_send_now_event(connectionHandle);
    }
}

//
// Created by Михайло Грошевий on 21/01/2025.
//

#include "Communication.h"

#include <pico/cyw43_arch.h>
#include "ble_config.h"

constexpr uint8_t advData[] = {
    0x02, 0x01, 0x06, // Length: 2 bytes, type: Bluetooth Data Type Flags (0x01), value: LE General Discoverable Mode + BR/EDR Not Supported (0x02 + 0x04)
    0x09, 0x09, 'P', 'i', 'c', 'o', ' ', '2', ' ', 'W', // Length: 9 bytes, type: Complete Local Name (0x09), value: Pico 2 W
    0x11, 0x07, // Length: 17 bytes, type: Complete List Of 128 Bit Service Class UUIDs (0x07)
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x10, 0xff, 0x00, 0x00  // value: 0000FF10-0000-1000-8000-00805F9B34FB
};

void Communication::init() {
    cyw43_arch_init();
    l2cap_init();
    sm_init();

    att_server_init(profile_data, attReadCallback, attWriteCallback);

    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, {}, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(advData), (uint8_t*) advData);
    gap_advertisements_enable(1);

    hci_power_control(HCI_POWER_ON);
}

void Communication::setCallback(const std::function<void(int, float)> &newCallback) {
    callback = newCallback;
}

void Communication::setCurrentValue(const std::string& newValue) {
    currentValue = newValue;
}

void Communication::sendNotification(const std::string& value) {
    if (notificationHandle == 0) {
        return;
    }
    att_server_notify(notificationHandle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE,
        (uint8_t*) value.c_str(), sizeof(value));
}

uint16_t Communication::attReadCallback(hci_con_handle_t connectionHandle, uint16_t attHandle,
                                        uint16_t offset, uint8_t* buffer, uint16_t bufferSize) {
    UNUSED(connectionHandle);

    if (attHandle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        return att_read_callback_handle_blob((const uint8_t*) currentValue.c_str(), currentValue.size(),
            offset, buffer, bufferSize);
    }
    return 0;
}

int Communication::attWriteCallback(hci_con_handle_t connectionHandle, uint16_t attHandle, uint16_t transactionMode,
                                    uint16_t offset, uint8_t* buffer, uint16_t bufferSize) {
    UNUSED(transactionMode);
    UNUSED(offset);

    if (attHandle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE) {
        if (little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION) {
            notificationHandle = connectionHandle;
        } else {
            notificationHandle = 0;
        }
    }

    if (attHandle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        std::string receivedString((char*) buffer, bufferSize);

        size_t colonPos = receivedString.find(':');
        int integerPart = std::stoi(receivedString.substr(0, colonPos));
        float floatPart = std::stof(receivedString.substr(colonPos + 1));

        callback(integerPart, floatPart);
    }
    return 0;
}

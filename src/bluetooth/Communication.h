//
// Created by Михайло Грошевий on 21/01/2025.
//

#pragma once

#include <functional>

#include <btstack.h>

#include "proto/triangle.pb.h"

class Communication {
public:
    struct Handlers {
        std::function<triangle_v1_Parameters()> readParameters;
        std::function<triangle_v1_Error(const triangle_v1_Parameters&)> writeParameters;
        std::function<triangle_v1_Error(const triangle_v1_Command&)> executeCommand;
    };

    static void init(const Handlers& newHandlers);

private:
    static uint16_t attReadCallback(hci_con_handle_t, uint16_t, uint16_t, uint8_t*, uint16_t);
    static int attWriteCallback(hci_con_handle_t, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t);
    static void packetHandler(uint8_t packetType, uint16_t channel, uint8_t* packet, uint16_t size);
    static void telemetryTimerHandler(btstack_timer_source_t* timer);
    static void sendTelemetry();

    static inline Handlers handlers;
    static inline hci_con_handle_t connectionHandle = HCI_CON_HANDLE_INVALID;
    static inline bool isTelemetryEnabled = false;

    static inline btstack_timer_source_t telemetryTimer;
    static inline btstack_packet_callback_registration_t hciEventRegistration;
};

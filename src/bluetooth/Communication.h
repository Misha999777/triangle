//
// Created by Михайло Грошевий on 21/01/2025.
//

#pragma once

#include <btstack.h>

#include "proto/triangle.pb.h"
#include "Telemetry.h"

class Communication {
public:
    class Delegate {
    public:
        virtual ~Delegate() = default;

        virtual triangle_v1_Parameters getParameters() = 0;
        virtual bool setParameters(const triangle_v1_Parameters& parameters) = 0;
        virtual bool saveParameters() = 0;
    };

    Communication(Delegate& delegate, Telemetry& telemetry);

    int onAttRead(int attHandle, int offset, uint8_t* buffer, int bufferSize);
    int onAttWrite(hci_con_handle_t connection, int attHandle, uint8_t* buffer, int bufferSize);
    void onDisconnected();
    void onTelemetryTimer();
    void onCanSendNow();

private:
    Delegate& delegate;
    Telemetry& telemetry;

    hci_con_handle_t telemetryConnection = HCI_CON_HANDLE_INVALID;
    btstack_timer_source_t telemetryTimer = {};
    btstack_context_callback_registration_t canSendNowRegistration = {};
    uint8_t telemetryPacket[triangle_v1_TelemetryBatch_size] = {};
};

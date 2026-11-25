//
// BLE wire format shared with the web app (web/src/services/robot.js). All values are little-endian.
//

#pragma once

#include <cstdint>

namespace Protocol {
    // Parameters characteristic (0xFF11), read and write: the whole struct at once.
    struct __attribute__((packed)) Parameters {
        float k1;
        float k2;
        float k3;
        float targetAngle;
    };

    // Command characteristic (0xFF13), write: a single byte.
    enum class Command : uint8_t {
        SAVE_PARAMETERS = 0x01,
    };

    // Application ATT error codes returned from writes.
    constexpr uint8_t ERROR_INVALID_VALUE   = 0x80;
    constexpr uint8_t ERROR_BUSY            = 0x81; // e.g. saving while balancing
    constexpr uint8_t ERROR_UNKNOWN_COMMAND = 0x82;

    // Telemetry characteristic (0xFF12), notify: a header followed by `count` consecutive samples.
    constexpr uint8_t TELEMETRY_VERSION = 1;

    struct __attribute__((packed)) TelemetryHeader {
        uint8_t version;
        uint8_t count;
        uint16_t sequence;         // sequence number of the first sample; consecutive samples follow
    };

    struct __attribute__((packed)) TelemetrySample {
        uint16_t timeMs;           // milliseconds since boot, wraps around
        int16_t angle;             // degrees * ANGLE_SCALE
        int16_t angularVelocity;   // degrees/s * ANGULAR_VELOCITY_SCALE
        int16_t wheelVelocity;     // radians/s * WHEEL_VELOCITY_SCALE
        int16_t torque;            // volts * TORQUE_SCALE
        int16_t targetAngle;       // degrees * ANGLE_SCALE
        uint8_t flags;             // FLAG_*
    };

    constexpr float ANGLE_SCALE            = 100.0f;
    constexpr float ANGULAR_VELOCITY_SCALE = 10.0f;
    constexpr float WHEEL_VELOCITY_SCALE   = 10.0f;
    constexpr float TORQUE_SCALE           = 1000.0f;

    constexpr uint8_t FLAG_BALANCING = 0x01;

    // The web app decodes these by byte offset.
    static_assert(sizeof(Parameters) == 16);
    static_assert(sizeof(TelemetryHeader) == 4);
    static_assert(sizeof(TelemetrySample) == 13);
}

#include "Telemetry.h"

#include <algorithm>
#include <cmath>

#include <pico/time.h>

namespace {
    int16_t scale(float value, float factor) {
        if (std::isnan(value)) return 0;
        return (int16_t) std::clamp(std::round(value * factor), -32768.0f, 32767.0f);
    }
}

void Telemetry::record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                       bool isBalancing) {
    // Every sample gets a sequence number, even a dropped one, so the receiver can see gaps.
    uint16_t sequence = nextSequence++;

    uint32_t currentHead = head.load(std::memory_order_relaxed);
    if (currentHead - tail.load(std::memory_order_acquire) >= CAPACITY) {
        return;
    }

    entries[currentHead % CAPACITY] = {
        sequence,
        {
            (uint16_t) to_ms_since_boot(get_absolute_time()),
            scale(angle, Protocol::ANGLE_SCALE),
            scale(angularVelocity, Protocol::ANGULAR_VELOCITY_SCALE),
            scale(wheelVelocity, Protocol::WHEEL_VELOCITY_SCALE),
            scale(torque, Protocol::TORQUE_SCALE),
            scale(targetAngle, Protocol::ANGLE_SCALE),
            (uint8_t) (isBalancing ? Protocol::FLAG_BALANCING : 0),
        },
    };
    head.store(currentHead + 1, std::memory_order_release);
}

size_t Telemetry::available() {
    return head.load(std::memory_order_acquire) - tail.load(std::memory_order_relaxed);
}

size_t Telemetry::read(Protocol::TelemetrySample* samples, size_t maxCount, uint16_t& firstSequence) {
    uint32_t currentTail = tail.load(std::memory_order_relaxed);
    uint32_t currentHead = head.load(std::memory_order_acquire);

    size_t count = 0;
    while (currentTail + count != currentHead && count < maxCount) {
        const Entry& entry = entries[(currentTail + count) % CAPACITY];
        if (count == 0) {
            firstSequence = entry.sequence;
        } else if (entry.sequence != (uint16_t) (firstSequence + count)) {
            break; // a gap from dropped samples starts a new packet
        }
        samples[count++] = entry.sample;
    }

    tail.store(currentTail + (uint32_t) count, std::memory_order_release);
    return count;
}

void Telemetry::clear() {
    tail.store(head.load(std::memory_order_acquire), std::memory_order_release);
}

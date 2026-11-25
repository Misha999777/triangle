//
// Lock-free single-producer/single-consumer buffer between the control loop and the BLE stack.
//

#pragma once

#include <atomic>
#include <cstddef>

#include "Protocol.h"

class Telemetry {
public:
    // Producer: the control loop. Never blocks; drops the sample if the buffer is full.
    static void record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                       bool isBalancing);

    // Consumer: the BLE stack.
    static size_t available();
    static size_t read(Protocol::TelemetrySample* samples, size_t maxCount, uint16_t& firstSequence);
    static void clear();

private:
    static constexpr size_t CAPACITY = 256; // 2.5 s at 100 Hz

    struct Entry {
        uint16_t sequence;
        Protocol::TelemetrySample sample;
    };

    static inline Entry entries[CAPACITY];
    static inline std::atomic<uint32_t> head{0}; // written only by the producer
    static inline std::atomic<uint32_t> tail{0}; // written only by the consumer
    static inline uint16_t nextSequence = 0;
};

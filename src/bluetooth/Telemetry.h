//
// Lock-free single-producer/single-consumer buffer between the control loop and the BLE stack.
//

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

struct TelemetrySample {
    uint32_t sequence;
    uint32_t timeMs;
    float angle;
    float angularVelocity;
    float wheelVelocity;
    float torque;
    float targetAngle;
    bool isBalancing;
};

class Telemetry {
public:
    // Producer: the control loop. Never blocks; drops the sample if the buffer is full.
    static void record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                       bool isBalancing);

    // Consumer: the BLE stack. Encodes as many buffered samples as fit in `maxSize` bytes into a
    // TelemetryBatch message and removes them. Returns the encoded size, or 0 if there was nothing to send.
    static size_t encodeBatch(uint8_t* buffer, size_t maxSize, size_t& sampleCount);
    static size_t available();
    static void clear();

private:
    // Copies up to `maxCount` consecutive samples (stopping at a gap left by dropped samples) without removing them.
    static size_t peek(TelemetrySample* samples, size_t maxCount);
    static void consume(size_t count);

    static constexpr size_t CAPACITY = 256; // 2.5 s at 100 Hz

    static inline TelemetrySample entries[CAPACITY];
    static inline std::atomic<uint32_t> head{0}; // written only by the producer
    static inline std::atomic<uint32_t> tail{0}; // written only by the consumer
    static inline uint32_t nextSequence = 0;
};

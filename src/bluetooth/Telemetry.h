//
// Created by Михайло Грошевий on 25/11/2026.
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
    static void record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                       bool isBalancing);

    static size_t encodeBatch(uint8_t* buffer, size_t maxSize, size_t& sampleCount);
    static size_t available();
    static void clear();

private:
    static size_t peek(TelemetrySample* samples, size_t maxCount);
    static void consume(size_t count);

    static constexpr size_t CAPACITY = 256; // 2.5 s at 100 Hz

    static inline TelemetrySample entries[CAPACITY];
    static inline std::atomic<uint32_t> head{0};
    static inline std::atomic<uint32_t> tail{0};
    static inline uint32_t nextSequence = 0;
};

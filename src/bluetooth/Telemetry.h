//
// Created by Михайло Грошевий on 25/11/2026.
//

#pragma once

#include <atomic>
#include <cstdint>

#include "proto/triangle.pb.h"

class Telemetry {
public:
    void record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                bool isBalancing);

    int encodeBatch(uint8_t* buffer, int maxSize, int& sampleCount);
    int available();
    void clear();

private:
    void consume(int count);

    static constexpr int CAPACITY = 256; // 2.5 s at 100 Hz

    triangle_v1_TelemetrySample entries[CAPACITY] = {};
    std::atomic<uint32_t> head{0};
    std::atomic<uint32_t> tail{0};
    uint32_t nextSequence = 0;

    triangle_v1_TelemetryBatch batch = triangle_v1_TelemetryBatch_init_zero;
};

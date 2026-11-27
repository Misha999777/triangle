//
// Created by Михайло Грошевий on 25/11/2026.
//

#include "Telemetry.h"

#include <algorithm>

#include <pb_encode.h>
#include <pico/time.h>

#define MAX_SAMPLES_PER_BATCH (int) (sizeof(triangle_v1_TelemetryBatch::samples) / sizeof(triangle_v1_TelemetrySample))

void Telemetry::record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                       bool isBalancing) {
    uint32_t sequence = nextSequence++;

    uint32_t currentHead = head.load(std::memory_order_relaxed);
    if (currentHead - tail.load(std::memory_order_acquire) >= CAPACITY) {
        return;
    }

    entries[currentHead % CAPACITY] = {
        sequence,
        to_ms_since_boot(get_absolute_time()),
        angle,
        angularVelocity,
        wheelVelocity,
        torque,
        targetAngle,
        isBalancing,
    };
    head.store(currentHead + 1, std::memory_order_release);
}

int Telemetry::available() {
    return (int) (head.load(std::memory_order_acquire) - tail.load(std::memory_order_relaxed));
}

void Telemetry::consume(int count) {
    tail.store(tail.load(std::memory_order_relaxed) + count, std::memory_order_release);
}

void Telemetry::clear() {
    tail.store(head.load(std::memory_order_acquire), std::memory_order_release);
}

int Telemetry::encodeBatch(uint8_t* buffer, int maxSize, int& sampleCount) {
    int count = std::min(available(), MAX_SAMPLES_PER_BATCH);
    if (count == 0) {
        return 0;
    }

    uint32_t currentTail = tail.load(std::memory_order_relaxed);
    for (int i = 0; i < count; i++) {
        batch.samples[i] = entries[(currentTail + i) % CAPACITY];
    }

    size_t encodedSize = 0;
    for (; count > 0; count--) {
        batch.samples_count = (pb_size_t) count;
        if (pb_get_encoded_size(&encodedSize, triangle_v1_TelemetryBatch_fields, &batch) && (int) encodedSize <= maxSize) {
            break;
        }
    }

    if (count == 0) {
        // Not even one sample fits (MTU too small): drop it so the buffer keeps moving.
        consume(1);
        return 0;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, maxSize);
    bool isEncoded = pb_encode(&stream, triangle_v1_TelemetryBatch_fields, &batch);
    consume(count);

    sampleCount = count;
    return isEncoded ? (int) stream.bytes_written : 0;
}

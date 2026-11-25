#include "Telemetry.h"

#include <pb_encode.h>
#include <pico/time.h>

#include "proto/triangle.pb.h"

namespace {
    constexpr size_t MAX_SAMPLES_PER_BATCH = sizeof(triangle_v1_TelemetryBatch::angle) / sizeof(float);

    void fillBatch(triangle_v1_TelemetryBatch& batch, const TelemetrySample* samples, size_t count) {
        batch = triangle_v1_TelemetryBatch_init_zero;
        batch.sequence = samples[0].sequence;
        batch.time_ms = samples[0].timeMs;
        for (size_t i = 0; i < count; i++) {
            batch.time_delta_ms[i] = i == 0 ? 0 : samples[i].timeMs - samples[i - 1].timeMs;
            batch.angle[i] = samples[i].angle;
            batch.angular_velocity[i] = samples[i].angularVelocity;
            batch.wheel_velocity[i] = samples[i].wheelVelocity;
            batch.torque[i] = samples[i].torque;
            batch.target_angle[i] = samples[i].targetAngle;
            batch.balancing[i] = samples[i].isBalancing;
        }
        batch.time_delta_ms_count = batch.angle_count = batch.angular_velocity_count = batch.wheel_velocity_count =
            batch.torque_count = batch.target_angle_count = batch.balancing_count = (pb_size_t) count;
    }
}

void Telemetry::record(float angle, float angularVelocity, float wheelVelocity, float torque, float targetAngle,
                       bool isBalancing) {
    // Every sample gets a sequence number, even a dropped one, so the receiver can see gaps.
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

size_t Telemetry::available() {
    return head.load(std::memory_order_acquire) - tail.load(std::memory_order_relaxed);
}

size_t Telemetry::peek(TelemetrySample* samples, size_t maxCount) {
    uint32_t currentTail = tail.load(std::memory_order_relaxed);
    uint32_t currentHead = head.load(std::memory_order_acquire);

    size_t count = 0;
    while (currentTail + count != currentHead && count < maxCount) {
        const TelemetrySample& entry = entries[(currentTail + count) % CAPACITY];
        if (count > 0 && entry.sequence != samples[0].sequence + count) {
            break; // a gap from dropped samples starts a new batch
        }
        samples[count++] = entry;
    }
    return count;
}

void Telemetry::consume(size_t count) {
    tail.store(tail.load(std::memory_order_relaxed) + (uint32_t) count, std::memory_order_release);
}

void Telemetry::clear() {
    tail.store(head.load(std::memory_order_acquire), std::memory_order_release);
}

size_t Telemetry::encodeBatch(uint8_t* buffer, size_t maxSize, size_t& sampleCount) {
    TelemetrySample samples[MAX_SAMPLES_PER_BATCH];
    size_t count = peek(samples, MAX_SAMPLES_PER_BATCH);
    if (count == 0) {
        return 0;
    }

    // Shrink the batch until it fits.
    static triangle_v1_TelemetryBatch batch;
    size_t encodedSize = 0;
    for (; count > 0; count--) {
        fillBatch(batch, samples, count);
        if (pb_get_encoded_size(&encodedSize, triangle_v1_TelemetryBatch_fields, &batch) && encodedSize <= maxSize) {
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
    return isEncoded ? stream.bytes_written : 0;
}

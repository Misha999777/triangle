# Software Architecture

The software is structured into modular components designed for the RP2350 dual-core architecture.

## Dual-Core Processing

*   **Core 0**: Handles the main stabilization control loop (`u = k1*angle + k2*gyro + k3*motor_speed`), IMU reading via `AngleSensor`, and BLE communication via `Communication`.
*   **Core 1**: Dedicated entirely to the high-frequency motor control loop. It reads the absolute angle from the `MotorSensor` and implements the Field Oriented Control (FOC) algorithms in `Motor` and drives the PWM via `Driver`.

## Directory Structure

*   **`src/main.cpp`**: The entry point. Initializes hardware, launches the motor loop on Core 1, and runs the main control loop on Core 0.
*   **`src/hardware/`**:
    *   `AngleSensor`: Handles MPU6050 communication, data fusion (Complementary filter), and calibration.
    *   `Motor`: Implements the FOC algorithm, controlling the 3-phase voltage to generate torque.
    *   `MotorSensor`: Reads absolute angle from the SPI magnetic encoder.
    *   `Driver`: Generates 3-phase PWM signals.
*   **`src/controller/`**:
    *   `Controller`: Implements the state-feedback controller logic to calculate torque and adjust `targetAngle`.
    *   `Storage`: Saves/Restores calibration data and tuning parameters to flash memory.
*   **`src/bluetooth/`**:
    *   `Communication`: Manages the BLE stack and GATT services: parameter reads and writes, commands, and the telemetry stream.
    *   `Telemetry`: Lock-free ring buffer that the control loop writes samples into and the BLE stack drains into protobuf batches.
*   **`proto/`**: `triangle.proto`, the BLE wire format shared with the web app (see [Tuning](../tuning.md#ble-protocol)). CMake generates the nanopb code for it at build time.

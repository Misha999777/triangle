# Triangle

Self-balancing reaction wheel robot project, powered by the **Raspberry Pi Pico 2 W** (RP2350).

<img width="320" height="320" alt="Triangle demo image" src="https://github.com/user-attachments/assets/0581bb6a-d331-43c2-a018-158344336e4d" />

## Features
-   **Field Oriented Control (FOC)**: Implements commutation for the BLDC motor using Space Vector Pulse Width Modulation (SVPWM) (via Inverse Park/Clarke transforms).
-   **Dual-Core Architecture**:
    -   **Core 0**: Handles the control loop, and BLE communication.
    -   **Core 1**: Dedicated to the high-frequency motor control loop (FOC/Commutation) to ensure smooth motor operation.
-   **Auto-Calibration**: Automatically calibrates the gyroscope on startup and supports motor encoder alignment.
-   **Bluetooth LE Tuning**: Modify gains (`k1`, `k2`, `k3`) and target angle wirelessly using a BLE-enabled device.

## About the Project
This site contains documentation on how to build, program, and tune the Triangle self-balancing robot. See the tabs above for detailed information on Hardware, Software Architecture, and Build Guides.

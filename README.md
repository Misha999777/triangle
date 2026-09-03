# Triangle

Firmware for a self-balancing reaction wheel robot, powered by the **Raspberry Pi Pico 2 W** (RP2350).

<img width="320" height="320" alt="Triangle demo image" src="https://github.com/user-attachments/assets/0581bb6a-d331-43c2-a018-158344336e4d" />

## Features
-   **Field Oriented Control (FOC)**: Implements commutation for the BLDC motor using Space Vector Pulse Width Modulation (SVPWM) (via Inverse Park/Clarke transforms).
-   **Dual-Core Architecture**:
    -   **Core 0**: Handles the control loop, and BLE communication.
    -   **Core 1**: Dedicated to the high-frequency motor control loop (FOC/Commutation) to ensure smooth motor operation.
-   **Auto-Calibration**: Automatically calibrates the gyroscope on startup and supports motor encoder alignment.
-   **Bluetooth LE Tuning**: Modify gains (`k1`, `k2`, `k3`) and target angle wirelessly using a BLE-enabled device.

## Hardware Requirements

### Microcontroller
-   **RP2350** (Raspberry Pi Pico 2 W)

### Sensors
-   **IMU**: MPU6050
    -   Communication: I2C
    -   SDA: `GPIO 26`
    -   SCL: `GPIO 27`
-   **Motor Encoder**: AS5048A
    -   Communication: SPI
    -   SCK: `GPIO 2`
    -   MOSI: `GPIO 3`
    -   MISO: `GPIO 4`
    -   CS: `GPIO 5`

### Actuator
-   **Driver**: DRV8313
    -   PWM Phase A: `GPIO 10`
    -   PWM Phase B: `GPIO 11`
    -   PWM Phase C: `GPIO 12`
    -   Enable Pin: `GPIO 13`

## Software Architecture
The software is structured into modular components:

-   **`src/main.cpp`**: The entry point. It initializes hardware, launches the motor loop on Core 1, and runs the main control loop on Core 0.
-   **`src/hardware/`**:
    -   `AngleSensor`: Handles MPU6050 communication, data fusion (Complementary filter), and calibration.
    -   `Motor`: Implements the FOC algorithm, controlling the 3-phase voltage to generate torque.
    -   `MotorSensor`: Reads absolute angle from the SPI magnetic encoder.
    -   `Driver`: Generates 3-phase PWM signals.
-   **`src/controller/`**:
    -   `Controller`: Implements the state-feedback controller `u = k1*angle + k2*gyro + k3*motor_speed`. It also handles logic to adjust `targetAngle`.
    -   `Storage`: Saves/Restores calibration data and tuning parameters to flash memory.
-   **`src/bluetooth/`**:
    -   `Communication`: Manages the BLE stack, GATT services, and handles incoming parameter updates.

## Building the Project
This project uses the **Raspberry Pi Pico SDK** and **CMake**.

1.  **Install Prerequisites**:
    -   CMake
    -   Arm Toolchain (`arm-none-eabi-gcc`)
    -   Raspberry Pi Pico SDK (Set `PICO_SDK_PATH` environment variable)

2.  **Build**:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

3.  **Flash**:
    -   Hold the BOOTSEL button on the Pico 2 W and plug it in via USB.
    -   Copy the generated `triangle.uf2` file to the mounted RPI-RP2 drive.

## Debugging
The project includes an `openocd_pico.cfg` configuration file for debugging with **OpenOCD** and **GDB** via the **SWD** interface.
It is configured for **CMSIS-DAP** adapters (e.g., Raspberry Pi Debug Probe or another Pico running `picoprobe`).

To start an OpenOCD session:
```bash
openocd -f openocd_pico.cfg
```

You can then connect with GDB:
```bash
arm-none-eabi-gdb build/triangle.elf
(gdb) target remote localhost:3333
```

## Bluetooth Interface
BLE service is exposed for real-time tuning. You can use a generic BLE Scanner or a custom software to connect.

### Telemetry
-   **Status String**: Contains Target Angle and Current Angle.

### Writable Parameters (Float)
-   **Index 1**: `k1` (Angle Gain)
-   **Index 2**: `k2` (Angular Velocity Gain)
-   **Index 3**: `k3` (Motor Speed Gain)
-   **Index 4**: `targetAngle` (Manual offset for balance point)

## License
See `LICENSE` file.

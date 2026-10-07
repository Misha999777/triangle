# Hardware Overview

This section covers the electronic and physical components of the Triangle robot.

## Wiring & Block Diagram

```mermaid
flowchart TD
    Battery["1S Li-Ion"]
    Charger["Charge & Protection"]
    DCDC["DC-DC Boost"]
    Pico["RP2350 (Pico 2 W)"]
    IMU["MPU6050 IMU"]
    DRV["DRV8313 Driver"]
    Motor["BLDC Motor"]
    Encoder["AS5048A Encoder"]

    Battery -->|"Direct Power"| Charger
    
    Charger -->|"1S Power"| DCDC
    Charger -->|"1S Power"| Pico
    
    DCDC -->|"Boosted Power"| DRV
    Pico -->|"PWM Control"| DRV
    
    Pico -->|"3.3V Power"| IMU
    Pico <-->|"I2C Data"| IMU
    
    DRV -->|"3-Phase Power"| Motor
    DRV -->|"3.3V Power"| Encoder
    
    Motor -.->|"Shaft Rotation"| Encoder
    Encoder -->|"SPI Data"| Pico
```

## Pinout Mapping

| Component   | Pin Function | GPIO Pin  |
| :---        | :---         | :---      |
| **MPU6050** | SDA          | `GPIO 26` |
|             | SCL          | `GPIO 27` |
| **AS5048A** | SCK          | `GPIO 2`  |
|             | MOSI         | `GPIO 3`  |
|             | MISO         | `GPIO 4`  |
|             | CS           | `GPIO 5`  |
| **DRV8313** | PWM A        | `GPIO 10` |
|             | PWM B        | `GPIO 11` |
|             | PWM C        | `GPIO 12` |
|             | EN           | `GPIO 13` |

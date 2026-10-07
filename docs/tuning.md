# Tuning

The Triangle robot features a Bluetooth Low Energy (BLE) interface for real-time telemetry and tuning without needing to recompile the firmware.

### Telemetry

*   **Status String**: Contains the `Target Angle` and `Current Angle` formatted as a string.

### Writable Parameters

These parameters allow you to tune the state-feedback controller `u = k1*angle + k2*gyro + k3*motor_speed`.

*   **Index 1**: `k1` (Angle Gain) - Adjusts the stiffness around the balance point.
*   **Index 2**: `k2` (Angular Velocity Gain) - Adds damping to prevent oscillation.
*   **Index 3**: `k3` (Motor Speed Gain) - Prevents the motor from reaching its maximum speed limit by leaning slightly in the direction of travel.
*   **Index 4**: `targetAngle` - Manual offset for the balance point. Use this to compensate for any physical imbalance in the chassis.

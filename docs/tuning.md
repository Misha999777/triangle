# Tuning

The Triangle robot features a Bluetooth Low Energy (BLE) interface for real-time telemetry and tuning without needing to recompile the firmware. The [Web App](software/web-app.md) provides a UI for all of it, right in the browser.

### Parameters

These parameters allow you to tune the state-feedback controller `u = k1*angle + k2*gyro + k3*motor_speed`.

*   `k1` (Angle Gain) - Adjusts the stiffness around the balance point.
*   `k2` (Angular Velocity Gain) - Adds damping to prevent oscillation.
*   `k3` (Motor Speed Gain) - Prevents the motor from reaching its maximum speed limit by leaning slightly in the direction of travel.
*   `targetAngle` - The balance point. Use this to compensate for any physical imbalance in the chassis. While balancing, the robot also nudges it on its own to keep the wheel from drifting.

Changes take effect immediately. **Save to flash** keeps them after a restart; the robot only accepts it while it isn't balancing, because writing flash pauses both cores.

### Telemetry

Every control loop iteration (100 Hz) is streamed: angle, angular velocity, wheel speed, motor torque, target angle and whether the robot is balancing.

### BLE Protocol

All characteristics belong to service `0000FF10-0000-1000-8000-00805F9B34FB`. Values are little-endian; the structs are defined in `src/bluetooth/Protocol.h`.

| Characteristic    | Access       | Payload                                                                                                                            |
|-------------------|--------------|------------------------------------------------------------------------------------------------------------------------------------|
| `FF11` Parameters | Read, Write  | `k1`, `k2`, `k3`, `targetAngle` as four `float32` (16 bytes). Writes must contain all four; non-finite values are rejected.        |
| `FF12` Telemetry  | Notify       | Header: `version` (u8, currently 1), `count` (u8), `sequence` (u16) of the first sample. Then `count` consecutive 13-byte samples. |
| `FF13` Command    | Write        | One byte: `0x01` saves the parameters to flash.                                                                                    |

Each telemetry sample contains `timeMs` (u16, milliseconds since boot, wrapping) followed by `int16` values scaled to fixed point: angle ×100 (°), angular velocity ×10 (°/s), wheel velocity ×10 (rad/s), torque ×1000 (V), target angle ×100 (°), and a `flags` byte (bit 0: balancing).

The control loop writes samples into a 2.5-second ring buffer, and the BLE stack drains it every 25 ms, packing as many samples into each notification as the connection's MTU allows. If the link can't keep up, new samples are dropped rather than blocking the control loop; the gap shows up in the sequence numbers.

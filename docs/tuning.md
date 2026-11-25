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

All characteristics belong to service `0000FF10-0000-1000-8000-00805F9B34FB`. Each one carries a [Protocol Buffers](https://protobuf.dev/) message defined in [`proto/triangle.proto`](https://github.com/Misha999777/triangle/blob/main/proto/triangle.proto), the single source of truth for both sides.

| Characteristic    | Access      | Message          | Notes                                                                                     |
|-------------------|-------------|------------------|-------------------------------------------------------------------------------------------|
| `FF11` Parameters | Read, Write | `Parameters`     | Reads return all fields. Writes may contain any subset; only the fields present change.   |
| `FF12` Telemetry  | Notify      | `TelemetryBatch` | Consecutive samples stored column by column, with the sequence number of the first one.   |
| `FF13` Command    | Write       | `Command`        | A `CommandType`; currently only `SAVE_PARAMETERS`, rejected while balancing.             |

Rejected writes return an ATT error code from the `Error` enum.

The control loop writes samples into a 2.5-second ring buffer, and the BLE stack drains it every 25 ms, packing as many samples into each notification as the connection's MTU allows (about 10 at the usual MTU of 247). If the link can't keep up, new samples are dropped rather than blocking the control loop; the gap shows up in the sequence numbers. A batch needs more than the minimum BLE MTU of 23 bytes, so the stream requires a client that negotiates a larger MTU, as desktop and Android browsers normally do.

### Changing the protocol

Edit `proto/triangle.proto` (and `proto/triangle.options` for nanopb array sizes). Both sides regenerate their code automatically, so nothing generated is kept in git:

*   **Firmware:** CMake generates [nanopb](https://jpa.kapsi.fi/nanopb/) code into `build/proto/` whenever the `.proto` changes (see `proto/CMakeLists.txt`).
*   **Web app:** `npm run dev` and `npm run build` generate [protobuf-es](https://github.com/bufbuild/protobuf-es) code into `web/src/gen/`.

Both use the installed `protoc` (see [Building](software/building.md#prerequisites)). The nanopb generator and its Python dependency are set up automatically in the build directory.

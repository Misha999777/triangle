# Web App

The web app connects to the robot straight from the browser via [Web Bluetooth](https://developer.mozilla.org/docs/Web/API/Web_Bluetooth_API). It plots live telemetry and sends tuning parameters, with nothing to install.

**[Open the Web App](https://misha999777.github.io/triangle/app/)**

## Requirements

*   Chrome or Edge on desktop or Android. Firefox and Safari don't support Web Bluetooth.
*   Bluetooth turned on, and the robot powered on and not connected to another device.

## Usage

1.  Click **Connect…** and pick **Triangle** in the browser's device picker.
2.  Above the chart are the latest angle, target, error, angular velocity, wheel speed and torque, plus the sample rate and the number of samples dropped on the way. Tap a value to show or hide it on the chart. Angles (angle, target, error), angular velocities (gyro and wheel, both in °/s) and torque each get their own stacked chart. The charts stream at 100 Hz over the last 30 seconds.
3.  The [parameters](../tuning.md) are loaded from the robot when you connect. Edit them and click **Apply** to send only the values you changed, so the robot keeps its own adjustments to the rest, or **Reload** to fetch the current values again.
4.  Click **Save to flash** to keep the values after a restart. It's disabled while the robot is balancing.

If the connection drops, the app keeps trying to reconnect until you click **Disconnect**.

## Architecture

| Layer | Tech                                                                                                        |
|-------|-------------------------------------------------------------------------------------------------------------|
| UI    | Vite, React, [Fluent UI](https://github.com/microsoft/fluentui), [uPlot](https://github.com/leeoniya/uPlot) |
| Link  | Web Bluetooth                                                                                               |

*   **`web/src/services/robot.js`**: all communication with the robot: connecting, decoding the [telemetry stream](../tuning.md#ble-protocol), reading and writing parameters, and reconnecting.
*   **`web/src/gen/`**: protobuf-es code generated from `proto/triangle.proto` by `npm run dev` and `npm run build` (see [Changing the protocol](../tuning.md#changing-the-protocol)). Requires `protoc` to be installed.
*   **`web/src/components/`**: the connection bar, the telemetry chart and the parameters panel.

## Development

```bash
cd web
npm install
npm run dev
```

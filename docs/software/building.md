# Building the Firmware

This project uses the **Raspberry Pi Pico SDK** and **CMake**.

## Prerequisites

Ensure you have the following installed on your system:

1.  CMake
2.  Ninja Build
3.  Arm Toolchain (`arm-none-eabi`)
4.  Raspberry Pi Pico SDK

**Environment Variables:**
Ensure `PICO_SDK_PATH` is set to the location of your pico-sdk.
Ensure `PICO_TOOLCHAIN_PATH` is set to your ARM toolchain directory.

## Build Instructions

1.  Clone the repository:
    ```bash
    git clone https://github.com/Misha999777/triangle.git
    cd triangle
    ```
2.  Create a build directory and run CMake:
    ```bash
    mkdir build
    cd build
    cmake ..
    ninja
    ```

## Flashing the Pico 2 W

1.  Hold the `BOOTSEL` button on the Pico 2 W.
2.  Plug it into your computer via USB.
3.  The Pico will mount as a mass storage device named `RPI-RP2`.
4.  Copy the generated `triangle.uf2` file from the `build/` directory to the `RPI-RP2` drive.
5.  The Pico will automatically reboot and start running the firmware.

## Debugging

If you are using a debug probe (like another Pico running Picoprobe or a CMSIS-DAP debugger):

First, start an OpenOCD session:
```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg
```

Then, connect with GDB in another terminal:
```bash
arm-none-eabi-gdb build/triangle.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) monitor reset init
(gdb) continue
```

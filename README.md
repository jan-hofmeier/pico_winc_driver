# Pico WINC1500 driver

Port of the WINC1500 driver to the Raspberry Pi Pico with simple http client example.

## Sources

* host_drv_19_3_0 -> https://github.com/arduino-libraries/WiFi101/releases/tag/0.7.0
* host_drv_19_7_7 -> https://github.com/MicrochipTech/WINC-Releases/tree/master/WINC1500/19_7_7
* http_client (iot) -> https://github.com/MicrochipTech/WINC15x0-HTTP-Client-Demo/tree/master
* docs/Atmel-42420-WINC1500-Software-Design-Guide_UserGuide.pdf -> https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42420-WINC1500-Software-Design-Guide_UserGuide.pdf

## Build Options

The following CMake parameters can be used to configure the build:

*   **`WINC_DRIVER_VERSION`**: Specifies the WINC driver version to use.
    *   **Values**: `"19_7_7"` or `"19_3_0"`
    *   **Example**: `cmake .. -DWINC_DRIVER_VERSION="19_7_7"`

*   **`BUILD_MODE`**: Determines the build configuration.
    *   **Values**: `"DRIVER"`, `"SIMULATOR"`, or `"COMBINED"`
    *   **Example**: `cmake .. -DBUILD_MODE="DRIVER"`

*   **`PICO_DEVICE_SERIAL`**: (Optional) Specifies the serial number of the Pico device to flash. If provided, `picotool` will target this specific device for `load` and `reboot` commands. You can find the device serial number using `picotool list`.
    *   **Example**: `cmake .. -DPICO_DEVICE_SERIAL="E6613800000000000000000000000000"` (This will use `--ser` flag for `picotool`).

### Flashing

After configuring and building, you can flash the binary to your Pico using:
`make flash`
If `PICO_DEVICE_SERIAL` was set during CMake configuration, it will use the specified device. Otherwise, `picotool` will use its default behavior.
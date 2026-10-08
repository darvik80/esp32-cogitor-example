# ESP32 → Cogitor IoT Platform example

A minimal ESP32-S3 firmware on [Embed Framework](https://github.com/darvik80/embed-framework) that connects to [Cogitor IoT Platform](https://cogitor.dev/iot-platform):

- sends telemetry every 10 seconds: heap, uptime, Wi-Fi signal (`wifiRssi`);
- reports its attributes (firmware version, chip, MAC, ESP-IDF version) on connect;
- drives a WS2812 LED strip through RPC: `led_attach`, `led_detach`, `set_led`;
- sends a `button` event when the BOOT button is pressed.

Step-by-step guide: [Connecting an ESP32 to Cogitor IoT Platform](https://cogitor.dev/articles/esp32-iot-platform).
Rules on top of this device (Wi-Fi level on the strip): [Node-RED and Cogitor IoT Platform](https://cogitor.dev/articles/node-red).

## Hardware

- ESP32-S3 board with 4 MB flash or more
- optional: WS2812 strip, data line on any free GPIO (17 in the guides), 5 V and GND

## Build

ESP-IDF 5.5 or newer. Embed Framework components are fetched from GitHub by the component manager on the first build.

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build flash monitor
```

In `menuconfig`:

| Menu | Setting |
|------|---------|
| Embed Framework — WiFi | SSID and password |
| Embed Framework — Cogitor IoT | Product ID, Device ID, MQTT broker host, Device access token, short topics |
| Example — button | Button GPIO (0 = BOOT) |

The values go to the local `sdkconfig`, which is in `.gitignore`. On the first boot the firmware copies them to the `fctry` NVS partition, so later builds may leave the token empty.

## RPC methods

| Method | Params | What it does |
|--------|--------|--------------|
| `led_attach` | `gpio`, `count`, `brightness` (optional) | Attaches a strip; calling it again is safe |
| `led_detach` | `gpio` | Detaches the strip |
| `set_led` | `gpio`, `offset`, `length`, `r`, `g`, `b`, `on` | Paints or turns off a range of LEDs |
| `rpc-list` | — | Built-in: lists methods and their params |

## Layout

```
main/main.cpp             services wiring
main/led_rpc_service.*    RPC methods for the LED strip
main/button_service.*     button → platform event
partitions.csv            4 MB: app + fctry for credentials
sdkconfig.defaults        defaults without secrets
```

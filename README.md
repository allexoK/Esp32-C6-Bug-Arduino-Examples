# Esp32-C6-Bug-Arduino-Examples
Hello Internet traveller!
You have just arrived to the repository containing Arduino code examples for Esp32-C6-Bug.
## Arduino IDE support
The official Arduino support for the Esp32-C6 chip is now ready (check out arduino-esp32 version 3.0.0). To install it you should insert index.json link(Stable -https://espressif.github.io/arduino-esp32/package_esp32_index.json or Development (Supports Esp32-C6 as of 5.2.2024) - https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json)  from https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
Into the preferences tab of Arduino IDE. After it's done the core will be available to download via Boards Manager.
Some examples are covered more in details in [Esp32-C6-Bug datasheet](https://github.com/allexoK/Esp32-C6-Bug-Docs/blob/main/esp32c6bugdatasheet.pdf)
When compiling the examples ensure that:
- The Board option in tools tab is set to 'ESP32C6 Dev Module'
- USB CDC On Boot option in tools tab is set to 'True'
- Port option in tools tab is set to the port, where you connected your board.
## ESP-IDF examples
If you are also interested in advanced features of Esp32-C6 like Zigbee and thread please refer to [Official examples](https://github.com/espressif/esp-idf/tree/release/v5.1/examples) and to Demos section of [Esp32-C6-Bug datasheet](https://github.com/allexoK/Esp32-C6-Bug-Docs/blob/main/esp32c6bugdatasheet.pdf) 

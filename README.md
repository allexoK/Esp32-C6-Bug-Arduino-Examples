# Esp32-C6-Bug-Arduino-Examples
Hello internet traveller,
You have just arrived to the repository containing Arduino code examples for Esp32-C6-Bug.
## Arduino IDE support
The official Arduino support for the Esp32-C6 chip is on the way(as of 13/08/2023). If you are viewing this repo and it is still not published you can go to [this manual](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) and refer to 'Windows (manual installation)' section to install Esp32-Arduino core supporting Esp32-C6. Here is how I did it:
- Download and install git
- Open git bash
- Go to [sketchdirectory]/hardware/espressif/(if you don’t hardware/espressif path then create it)
- Clone the repository:
  - git clone https://github.com/espressif/arduino-esp32.git
- Go to the cloned repository:
  - cd arduino-esp32
- Switch branch:
  - git checkout idf-release/v5.1
- Run 
  - git submodule update --init --recursive
- Open [sketchdirectory]/hardware/espressif/arduino-esp32/tools and double-click get.exe
- After the process finishes go to Arduino IDE->Tools->Board->Esp32C6 Dev Module



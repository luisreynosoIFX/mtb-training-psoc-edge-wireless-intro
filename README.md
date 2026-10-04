# PSOC™ Edge Training - Wireless Introduction

This training provides an introduction to adding Wi-Fi and Bluetooth® connectivity to PSOC™ Edge E84 microcontrollers using the AIROC™ CYW55513 combo device, including Wi-Fi onboarding over Bluetooth® LE, Bluetooth® LE Audio, machine learning with MQTT connectivity using the Virtual Connectivity Manager (VCM), and low-power operation with the WLAN Low-Power Assistant (LPA).
The training includes hands-on labs for each topic, providing a great starting point to getting familiar with wireless connectivity on PSOC™ Edge.

## Device family
- [PSOC™ Edge](https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm)
- [AIROC™ CYW55513](https://www.infineon.com/part/CYW55513)

## How to use this training?
1. Download the training [content](#content).
2. Watch the video or review the presentation at your own pace.
3. Follow the step-by-step instructions in the training manual during the hands-on sections.
4. Use the provided source files if needed to verify the lab results.

## Training level
- E2: Intermediate

## Prerequisites
### Recommended trainings
- This training does not cover foundational concepts of ModusToolbox™, PSOC™ Edge, or wireless protocols.
- For an introduction to PSOC™ MCUs, including getting-started guides for ModusToolbox™, see the [PSOC™ Developer Journey](https://www.infineon.com/psocdeveloper).
- For PSOC™ Edge trainings, from beginner tutorials to advanced sessions, visit the [PSOC™ Edge E84 Training Collection](https://infineon-academy.csod.com/ui/lms-learner-playlist/PlaylistDetails?playlistId=8f04565f-88f4-4ca7-83b3-22e501656fbd).
- For Infineon Wi-Fi and Bluetooth® trainings, visit the [Introduction to Infineon Wi-Fi & Bluetooth Training Collection](https://training.infineon.com/video/0033a2c3-cfa6-40d2-9bde-780551f10c15).

### Tools (see [training manual](#content) for versions and installation instructions)
- [ModusToolbox™](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup) with [Visual Studio Code](https://code.visualstudio.com/) and the Infineon ModusToolbox™ for VS Code extension
- [Edge Protect Security Suite](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup)
- [ModusToolbox™ Programming tools](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup)
- [LLVM for Arm®](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/)
- [ModusToolbox™ Audio SW Codecs Tech Pack](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxpackaudioswcodecs)
- [Nmap](https://nmap.org/download)
- [Wireshark](https://www.wireshark.org/download.html) (optional)
- [Mosquitto MQTT broker](https://mosquitto.org/download) (optional)
- [MQTT Explorer](https://github.com/thomasnordquist/MQTT-Explorer/releases) (optional)
- AIROC™ Bluetooth® Connect mobile application (Android or iOS)
- Terminal emulator

## Hardware
- [KIT_PSE84_EVAL](https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL) (required for Labs 2 and 4; Labs 1 and 3 can also use the KIT_PSE84_AI)
- [KIT_PSE84_AI](https://www.infineon.com/evaluation-board/KIT-PSE84-AI) (optional, Labs 1 and 3 only)
- USB-C cable
- Wi-Fi access point (router)
- Android or iOS mobile device (Lab 1)

## Duration
- 4hrs, including video and hands-on labs

## Agenda
1. Edge AI meets connectivity
2. Lab 1: Wi-Fi onboarding using BLE
3. Efficient Bluetooth® LE audio
4. Lab 2: Bluetooth® LE Audio
5. Dual core support with Virtual Connectivity Manager (VCM)
6. Lab 3: Adding connectivity to machine learning with dual-core virtual MQTT client
7. Low-power assistant (LPA)
8. Lab 4: WLAN Low-Power Assistant
9. Other key advantages

## Expected Outcomes
- Understand how PSOC™ Edge E84 and the CYW55513 combo device work together to provide Wi-Fi and Bluetooth® connectivity.
- Provision Wi-Fi credentials over Bluetooth® LE using a custom GATT service.
- Run a Bluetooth® LE Audio broadcast sink and understand how the Arm® Helium extension accelerates LC3 audio decoding.
- Add Wi-Fi and MQTT connectivity to a machine learning application using the Virtual Connectivity Manager (VCM).
- Use the Low-Power Assistant (LPA) offloads to keep the host MCU in Deep Sleep, and observe the effect with nping and Wireshark.

## Content
- Training video at Infineon Academy (coming soon)
- [Presentation](./Presentation/PSE84_wireless_intro_E2.pdf)
- [Training manual document](./Manual/pse84-wireless-intro-e2-training-manual.md)
  - [Training manual web page](https://infineon.github.io/mtb-training-psoc-edge-wireless-intro)
- [Lab solutions](https://github.com/Infineon/mtb-training-psoc-edge-wireless-intro/tree/main/Lab_Source)

## References and resources
- [PSOC™ Edge MCUs](https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm)
- [AIROC™ CYW55513](https://www.infineon.com/part/CYW55513)
- [PSOC™ Developer Journey](https://www.infineon.com/psocdeveloper)
- [PSOC™ Edge E84 Training Collection](https://infineon-academy.csod.com/ui/lms-learner-playlist/PlaylistDetails?playlistId=8f04565f-88f4-4ca7-83b3-22e501656fbd)
- [Introduction to Infineon Wi-Fi & Bluetooth Training Collection](https://training.infineon.com/video/0033a2c3-cfa6-40d2-9bde-780551f10c15)
- [AN241681: Low-power system design with PSOC™ Edge MCU and AIROC™ Wi-Fi & Bluetooth® combo chip](https://www.infineon.com/AN241681)
- [ModusToolbox™ Audio SW Codecs Tech Pack](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxpackaudioswcodecs)
- [PSOC™ Edge MCU: Wi-Fi dual-core virtual MQTT client](https://github.com/Infineon/mtb-example-psoc-edge-wifi-dual-core-virtual-mqtt-client)
- [DEEPCRAFT™ AI Hub](https://www.infineon.com/psocdeveloper/ai-hub)

## History

| Date | Version | Description |
| ---- | ------- | ----------- |
| 10/04/2026 | ** | First public release |

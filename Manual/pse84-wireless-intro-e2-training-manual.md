# PSOC™ Edge E84 wireless introduction training manual

## About this document

This is the lab manual for the PSOC™ Edge E84 Wireless Introduction training.

## Scope and purpose

This training manual provides a comprehensive guide to getting started with Wi-Fi and Bluetooth® applications using the PSOC™ Edge E84 MCU, including detailed instructions on creating, configuring, building, and running application code examples using ModusToolbox™.

## Intended audience

This manual is intended for embedded developers, application engineers, and technical users who are new to Wi-Fi and Bluetooth® connectivity with PSOC™ Edge and ModusToolbox™.

### Contents

- [About this document](#about-this-document)
- [Scope and purpose](#scope-and-purpose)
- [Intended audience](#intended-audience)
- [Contents](#contents)
- [Introduction](#introduction)
- [Required development tools and prerequisites](#required-development-tools-and-prerequisites)
- [Lab 1: Wi-Fi onboarding using BLE](#lab-1-wi-fi-onboarding-using-ble)
- [Lab 2: Bluetooth® LE Audio](#lab-2-bluetooth-le-audio)
- [Lab 3: Adding connectivity to machine learning with dual-core virtual MQTT client](#lab-3-adding-connectivity-to-machine-learning-with-dual-core-virtual-mqtt-client)
- [Lab 4: WLAN Low-Power Assistant](#lab-4-wlan-low-power-assistant)
- [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox)
- [Appendix B: KIT_PSE84_EVAL details](#appendix-b-kit_pse84_eval-details)
- [Appendix C: KIT_PSE84_AI details](#appendix-c-kit_pse84_ai-details)
- [Revision history](#revision-history)
- [Disclaimer](#disclaimer)

## Introduction

This training introduces the PSOC™ Edge E84 platform and the CYW55513 Wi-Fi & Bluetooth® combo device, and demonstrates how to use them together to build connected, machine-learning–enabled applications.

**PSOC™ Edge** combines high-performance Arm® Cortex®-M CPUs (a Cortex®-M55 with Helium DSP acceleration and an Ethos-U55 NPU, paired with a low-power Cortex®-M33) with dedicated AI/ML, graphics, and audio subsystems. This dual-core architecture lets designers dedicate one core to always-on tasks such as connectivity and system control, while the second core focuses on compute-intensive workloads such as inference or DSP, without one workload starving the other. It is well suited for use cases such as human-machine interfaces, remote sensing, condition monitoring, and other data-driven edge workloads.

The **CYW55513** is a flexible, production-grade Wi-Fi and Bluetooth® combo solution that pairs well with PSOC™ Edge for both prototyping and deployment. It provides robust wireless connectivity for applications that require reliable IP networking, over-the-air updates, cloud integration, or peripheral/device communication over Bluetooth. It also includes power-management features, through the Low-Power Assistant middleware, that let the host MCU remain in deep sleep for long periods while the combo device filters unwanted network traffic. Used as a wireless companion to PSOC™ Edge, it allows designers to add standards-based connectivity without overloading the main MCU and while keeping a clean separation between application processing and radio functionality.

Throughout this training, you will gain practical, hands-on experience with the PSOC™ Edge E84 evaluation kit and the CYW55513-based wireless subsystem, using them not only to establish wireless connectivity but also to develop a clear understanding of the key capabilities and advantages of both devices.

By the end of the labs, attendees will be able to:

- Set up and configure the PSOC™ Edge E84 hardware and associated peripherals.
- Integrate and configure the CYW55513 wireless device for Wi-Fi and Bluetooth® connectivity in PSOC™ Edge projects.
- Create and build PSOC™ Edge applications in ModusToolbox™, including selecting and instantiating example projects.
- Establish end-to-end wireless links suitable for cloud-connected, sensor-enabled, or interactive applications.

These skills prepare participants to design and prototype their own connected edge systems using PSOC™ Edge with CYW55513 as the connectivity solution.

## Required development tools and prerequisites

### Tools

#### Hardware

This training guide uses the PSOC™ Edge E84 Evaluation Kit and/or the PSOC™ Edge E84 AI Kit. Refer to the lab instructions for details.

- [PSOC™ Edge E84 Evaluation Kit](https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL) (KIT_PSE84_EVAL)
    - Ensure device boots from external memory: BOOT SW/SW6 in the 'ON'/'HIGH' position
    - Disable the alternate serial interface configuration: J20/J21 in the Tristate/Not-Connected (NC) position
    - See [Appendix B: KIT_PSE84_EVAL details](#appendix-b-kit_pse84_eval-details) for more details
- [PSOC™ Edge E84 AI Kit](https://www.infineon.com/evaluation-board/KIT-PSE84-AI) (KIT_PSE84_AI)
    - See [Appendix C: KIT_PSE84_AI details](#appendix-c-kit_pse84_ai-details) for more details

#### Software

- ModusToolbox™ software v3.9 or later
    - Recommended installation via the ModusToolbox™ Setup tool
- Visual Studio Code with the **Infineon ModusToolbox™ for VS Code** extension v1.10.0 or later
    - Install [Visual Studio Code](https://code.visualstudio.com/), then install the extension from the VS Code Marketplace. The extension can also install the ModusToolbox™ software for you if it is not already present

> **Note:** Command-line (CLI) instructions are also provided in [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox).

- Edge Protect Security Suite v2.2.0 or later
    - Installed by the ModusToolbox™ Setup tool as a dependency to ModusToolbox™
- ModusToolbox™ Programming Tools v1.9.0 or later
    - Installed by the ModusToolbox™ Setup tool as a dependency to ModusToolbox™
- Board support package (BSP)
    - KIT_PSE84_EVAL_EPC2 and/or KIT_PSE84_AI
- LLVM for Arm® v19.1.5 or later, if recommended by ModusToolbox™
    - See [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox) for LLVM installation and configuration steps
- ModusToolbox™ Audio SW Codecs Tech Pack v1.0.2 or later
    - Recommended installation via the ModusToolbox™ Setup tool
- Nmap v7.98 or later
    - Available at [nmap.org/download](https://nmap.org/download)
- [Optional] Mosquitto MQTT broker
    - Download from [mosquitto.org/download](https://mosquitto.org/download). This is only needed when replicating the MQTT test on a local bench; it is not required for the live training at DevCon.
- [Optional] MQTT Explorer
    - Download from the [MQTT Explorer releases page](https://github.com/thomasnordquist/MQTT-Explorer/releases). This is only needed when replicating the MQTT test on a local bench; it is not required for the live training at DevCon.
- [Optional] Wireshark v4.6.3 or later
    - Available at [wireshark.org/download](https://www.wireshark.org/download.html)
- Serial terminal emulator
    - Use Tera Term, PuTTY, or a similar terminal emulator

> **Note:** Different versions of tools and evaluation kits are expected to work with minor or no changes.

### Prerequisites

- Install the software and obtain the hardware listed above.
- This is an introductory training to Wi-Fi and Bluetooth® connectivity with PSOC™ Edge and ModusToolbox™; however, it is not intended to cover all basic concepts of PSOC™ Edge, ModusToolbox™, or wireless protocols.
    - For an introduction to PSOC™, including a getting started guide to ModusToolbox™, visit the [PSOC™ Developer Journey](https://www.infineon.com/psocdeveloper).
    - For PSOC™ Edge trainings, from beginner tutorials to advanced trainings, go to [PSOC™ Edge Training Collection](https://infineon-academy.csod.com/ui/lms-learner-playlist/PlaylistDetails?playlistId=8f04565f-88f4-4ca7-83b3-22e501656fbd).
    - For Infineon Wi-Fi and Bluetooth® trainings, visit the [Introduction to Infineon Wi-Fi & Bluetooth Training Collection](https://training.infineon.com/video/0033a2c3-cfa6-40d2-9bde-780551f10c15).

## Lab 1: Wi-Fi onboarding using BLE

### Objective

In this lab, you will learn how to securely provision Wi-Fi credentials onto PSOC™ Edge over Bluetooth® LE, using the AIROC™ Bluetooth® Connect mobile application and a custom GATT service, avoiding the need to hardcode Wi-Fi credentials in the application.

### Description

This code example uses the CM33 CPU of PSOC™ Edge to communicate with the AIROC™ CYW55513 combo device and control both the Wi-Fi and Bluetooth® LE functionality. Bluetooth® LE is used to securely exchange the Wi-Fi SSID and password with a mobile application through a custom GATT service and characteristics, so the device can then connect to the Wi-Fi access point (AP).

This example has a three-project structure (`proj_cm33_s`, `proj_cm33_ns`, and `proj_cm55`); all three projects are programmed to the external QSPI flash memory and executed in Execute-in-Place (XIP) mode. Extended boot launches the CM33 secure project, which configures the protection settings, launches the CM33 non-secure application, and enables the CM55 CPU.

Wi-Fi onboarding using Bluetooth® LE is a common pattern for production IoT devices, since it lets an installer or end user provide network credentials through a mobile app instead of embedding them in firmware or requiring a wired/serial connection.

### Hardware diagram

This lab can use the PSOC™ Edge E84 EVK or the PSOC™ Edge E84 AI Kit.

The main hardware components utilized by this lab are:

- UART: output information to serial terminal
- UART: Bluetooth® channel between PSOC™ Edge and CYW55513
- SDIO: Wi-Fi communication between PSOC™ Edge and CYW55513
- `USER_BTN1`: clears the stored Wi-Fi credentials from non-volatile memory (NVM)

Below is the hardware block diagram for this example using the PSOC™ Edge E84 AI Kit (top) and EVK (bottom):

<img src="assets/images/image_lab1_ai.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

<img src="assets/images/image_lab1_evk.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

> **Note:** For the KIT_PSE84_EVAL_EPC2 EVK, set BOOT SW to ON. See [Appendix B: KIT_PSE84_EVAL details](#appendix-b-kit_pse84_eval-details).
> This switch setting is not required for the KIT_PSE84_AI AI Kit.

### Project creation

1. Follow the steps in [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox) to create a new application.
   When creating the application, select the **PSOC™ Edge MCU: Wi-Fi onboarding using Bluetooth® LE** application under the **Bluetooth®** category

> **Warning:** Windows has a 260-character path length limit. A long workspace path and/or a long project name may cause build issues when creating some applications.

<img src="assets/images/image_lab1_wifi_onboarding_template.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. After opening the project in VS Code, go to the **Application** tab in the **ModusToolbox™ for VS Code** extension, and select the **proj_cm33_ns** application.
   Then, scroll down to **Tools** and click **Bluetooth® Configurator**

<img src="assets/images/image_lab1_bt_config.gif" alt="Figure" style="width:700px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. In the **Bluetooth® Configurator**, go to **GAP Settings**, select the **General** settings, and change **Device name** to a value of your preference.
   Note that this step is optional; however, it helps identify your device

   > **Note:** The advertisement packet has a maximum length. The application will show an error if the device name exceeds the maximum length.

<img src="assets/images/image_lab1_bt_config_name.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

4. **Save** the file to regenerate the corresponding files and close **Bluetooth® Configurator**
5. Open `mtb_shared/wifi-host-driver/release-<version>/generated_mac_address.txt` and `NVRAM_GENERATED_MAC_ADDRESS` to assign a MAC address

> **Note:** This step is optional when testing one device, but it is necessary when connecting multiple EVKs to the same router. CYW55513 contains OTP bits which are programmed during production by the OEM or module partner. The OTP bits are not programmed in the EVK, so the device reads the MAC address from the file above, or you can program the OTP bits using APIs.

6. Select the main **Application** in the **ModusToolbox™ for VS Code** extension and click the **Program** button to program the application. Note that this step will also build the application before programming it. 
   Make sure the top-level application, not an individual project, is selected to ensure all three projects (`cm33_s`, `cm33_ns`, and `cm55`) are built and programmed together

<img src="assets/images/image_lab1_program_application.png" alt="Figure" style="width:500px; max-width:100%; height:auto; display:block; margin:0 auto;" />

7. After the program runs successfully, open the serial terminal and configure the baud rate to 115200-8-N-1. Then press the kit reset button

Observe that the device initializes the Bluetooth® stack and starts advertising.

<img src="assets/images/image_lab1_output.gif" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

### Onboarding Wi-Fi credentials over Bluetooth® LE

1. Install the **AIROC™ Bluetooth® Connect** mobile application on your Android or iOS device, and turn on Bluetooth® and Location services

<img src="assets/images/image_lab1_airoc.png" alt="Figure" style="width:160px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. Press the reset switch (XRES) on the kit to restart advertising
3. Launch the AIROC™ Bluetooth® Connect app, and swipe down on the home screen to scan for Bluetooth® LE peripherals. Select your device (such as **bleProv1** as selected in **Bluetooth® Configurator**) to connect

<img src="assets/images/image_lab1_airoc_ble1.png" alt="Figure" style="width:400px; max-width:100%; height:auto; display:block; margin:0 auto;" />

4. Select the **GATT DB profile** from the carousel view, and select the service shown (custom Wi-Fi onboarding service)
5. Click the **Notify** button to enable notifications on the characteristic ending in **66** (Wi-Fi scan results)

<img src="assets/images/image_lab1_notify66.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

6. Then, select the characteristic ending in **67** (Wi-Fi control). Click the **Notify** button, and **Write** a hex value of **2** to start a Wi-Fi scan

<img src="assets/images/image_lab1_notify67_1.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

7. The device starts scanning and sends the network details as notifications in the characteristic with UUID ending in 66. The terminal prints the available Wi-Fi networks as shown below

<img src="assets/images/image_lab1_notify67_2.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

8. Using this same approach, provide the Wi-Fi credentials to the device by sending the SSID in ASCII or hex using the characteristic ending in **63**, and the password to the characteristic ending in **64**.
   Note that the application will ask for a passkey. Write the passkey shown in the terminal

<img src="assets/images/image_lab1_passkey.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

9. After sending the SSID and password, go to the characteristic ending in **67**. 
   Enable notifications (if not enabled already) and write the hex value as **1** to this characteristic to connect to the Wi-Fi network. 
   If the connection is successful, the server sends a notification with a value of 1, otherwise with a value of 0. The terminal will also show the results

<img src="assets/images/image_lab1_notify67_3.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

   > **Note:** Once a successful connection is established, the SSID and password are stored in non-volatile memory (NVM) and are used automatically on subsequent boots. Press **USER_BTN1** to erase the stored credentials.

### Modify the example: indicate Wi-Fi connection status with an LED

The connection result (success or failure) is already available in the code as the value written to the notification characteristic ending in `67`. This modification reuses that same result to drive `CYBSP_USER_LED1`, giving a visible status indicator in addition to the terminal output.

1. Open `proj_cm33_ns/main.c` and add the following code in `main()` to initialize the GPIO driving the LED

   > **Note:** This document includes code snippets using C and diff syntax. The snippet below uses C syntax allowing for easier copying.
```c
    Cy_GPIO_Pin_FastInit(CYBSP_USER_LED1_PORT, 
                        CYBSP_USER_LED1_PIN, 
                        CY_GPIO_DM_STRONG, 
                        CYBSP_LED_STATE_OFF, 
                        HSIOM_SEL_GPIO);
```

2. Locate the function `wifi_task()` in `proj_cm33_ns/wifi_task.c`. This function handles the Wi-Fi connection request and writes the result back to the characteristic ending in `67`.
   Add the following code to drive the LED with the connection result:

   > **Note:** This document includes code snippets using C and diff syntax. The snippet below uses diff syntax to highlight difference between removed (red) and added (green) lines.

* Modification #1:
```diff
            if(CY_RSLT_SUCCESS == result)
            {
                printf("Successfully joined the Wi-Fi network\n");
+                Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, CYBSP_LED_STATE_ON);

                /* Update GATT DB about connection */
                app_custom_service_wifi_control[0] = WIFI_CONTROL_CONNECT;
```

* Modification #2:

```diff
                else /* Notification not sent */
                {
                    printf("Notification not sent\n");
                }

                printf("Failed to join Wi-Fi network\n");
+                Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, CYBSP_LED_STATE_OFF);
```

3. Save the file, rebuild, and reprogram the application.
   Repeat the onboarding flow from the previous section. `CYBSP_USER_LED1` should turn on when the Wi-Fi connection succeeds, and turn (or stay) off if it fails

### Conclusion

You successfully used Bluetooth® LE and a custom GATT service to provision Wi-Fi credentials onto PSOC™ Edge without hardcoding them in the application or requiring a wired connection, using the AIROC™ Bluetooth® Connect mobile app to scan, select, and connect to a Wi-Fi network.

This demonstrates a practical onboarding flow for connected products, and shows how PSOC™ Edge and CYW55513 can combine Bluetooth® LE and Wi-Fi on the same CM33 core to simplify production and field deployment.

From a product perspective, this onboarding flow is what end users expect from modern IoT devices: no serial cable, no pre-provisioned network credentials, and no Wi-Fi password exposed in the firmware image. It also removes the need for a display or keypad dedicated to network setup, reducing bill-of-materials and industrial-design cost, and lowers support costs since customers can reconfigure Wi-Fi credentials themselves whenever they change routers or relocate the device.

## Lab 2: Bluetooth® LE Audio

### Objective

In this lab, you will explore Bluetooth® Low Energy (BLE) Audio using both broadcast source and broadcast sink code examples, with the PSOC™ Edge E84 MCU as the Bluetooth® host and AIROC™ CYW55513 Wi-Fi and Bluetooth® combo chip as the Bluetooth® controller. The source broadcasts audio, and the sink receives, decodes, and plays it through the EVK speaker.

> **Note:** At DevCon 2026, the presenter will implement and run the broadcast source. Participants will run the broadcast sink and listen to the presenter's audio broadcast. The source example and instructions are included for convenience so participants can reproduce the complete setup at a later time.

### Description

This lab demonstrates Bluetooth® Low Energy (BLE) Audio using the PSOC™ Edge E84 MCU as the Bluetooth® host and the AIROC™ CYW55513 Wi-Fi and Bluetooth® combo device as the Bluetooth® controller. In this architecture, PSOC™ Edge manages the audio application and system control, while CYW55513 provides the underlying Bluetooth radio and controller functionality required for BLE Audio streaming.

**BLE Audio** is increasingly important because it enables high-quality audio with significantly lower power consumption than legacy Bluetooth® Classic audio, making it well-suited for earbuds, hearing aids, wearables, and battery-powered speakers. It also supports advanced capabilities such as multi-stream audio (for improved stereo and seamless earbud switching) and broadcast audio (e.g., Auracast™-style use cases), which allow one transmitter to serve many listeners—something Bluetooth Classic handles less efficiently.

Compared with Bluetooth® Classic audio, BLE Audio offers:

- Lower energy usage for longer battery life.
- More flexible topologies (multi-stream and broadcast scenarios).
- Better scalability for multi-device and multi-listener environments.

On the processing side, the lab highlights how Infineon’s **library of audio codecs and signal-processing components** can be used on PSOC™ Edge to implement modern audio pipelines efficiently.

By leveraging the **Arm® Helium (M-Profile Vector Extension)** support in PSOC™ Edge’s Cortex-M55 core, these codecs and audio algorithms can be vectorized to accelerate operations such as encoding, decoding, filtering, and basic audio effects. This results in higher throughput, lower latency, and reduced CPU loading, leaving more headroom for other tasks such as user interface or machine-learning-based features.

Through this BLE Audio lab, attendees will learn how to combine PSOC™ Edge E84 and CYW55513 to implement a practical BLE Audio path, and how to take advantage of Infineon’s codec libraries and Helium acceleration to build efficient, low-power wireless audio solutions.

### Hardware diagram

Both BLE Audio examples in this lab are intended to run only on the PSOC™ Edge E84 EVK (KIT_PSE84_EVAL_EPC2), not the AI Kit. The lab uses the EVK speaker to play audio on the broadcast sink.

During DevCon 2026, each participant needs one EVK for the sink; the presenter supplies the source. To run both examples independently after the event, use two EVKs: one for the source and one for the sink. Keep the source powered and broadcasting while testing the sink.

The main hardware components utilized by this lab are:

- UART: output information to serial terminal
- UART: Bluetooth® channel between PSOC™ Edge and CYW55513
- I2S: speaker driving audio output for sink example
- PDM/PCM: digital microphones for source example

Below is the hardware block diagram for this example:

<img src="assets/images/image_lab2_evk.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

> **Note:** For the KIT_PSE84_EVAL_EPC2 EVK, set BOOT SW to ON. See [Appendix B: KIT_PSE84_EVAL details](#appendix-b-kit_pse84_eval-details).

### Code example availability

> **Note:** The BLE Audio broadcast source and broadcast sink code examples are expected to be published in ModusToolbox™ (MTB) soon. Both examples will be included in the **Lab_Source** folder for DevCon 2026. For the event, use these supplied examples through **Browse for Application**, rather than searching the ModusToolbox™ example catalog.

The supplied examples are in `<Lab source>/lab2_ble_audio/psoc-edge-btstack-broadcast-source` and `<Lab source>/lab2_ble_audio/psoc-edge-btstack-broadcast-sink`. These are preview versions for the training; consult their included README files for version-specific requirements.

### Optional: Run the broadcast source (not required for DevCon)

The presenter performs this workflow during DevCon 2026. 
Participants can proceed directly to [Run the broadcast sink](#run-the-broadcast-sink). These source steps are provided for later use with a second EVK.

1. Follow [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox), select **KIT_PSE84_EVAL_EPC2**, and click **Browse for Application**. Select `<Lab source>/lab2_ble_audio/psoc-edge-btstack-broadcast-source`, then select its checkbox and click **Create**
   <img src="assets/images/image_lab2_source_import.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. Open the imported source application in VS Code. Open **Library Manager**, click **Update**, and wait for dependency resolution to complete. The source does **not** require the Audio SW Codecs Tech Pack; optional runtime encoding uses Google liblc3 v1.0.3
3. [INFO] Open `common.mk` and observe that `ENABLE_LC3_ENCODING=0`, which transmits the bundled pre-encoded LC3 audio sample. Also observe that `AUDIO_CONFIG=MONO` transmits one channel; use the same setting on the sink example
4. Select the top-level application in the **ModusToolbox™ for VS Code** extension, and click **Program** to build and program all three projects onto the source EVK
5. Open a serial terminal at **115200-8-N-1**, then press the kit reset button. Confirm that `PSOC Edge MCU: Bluetooth LE Audio Broadcast Source Example` appears.
   Wait for Bluetooth® initialization and automatic broadcasting. The message `Audio broadcasting started. Use a Broadcast Sink to listen.` confirms transmission, but does not confirm sink synchronization or audible playback. Keep the source EVK powered
   <img src="assets/images/image_lab2_source_output.gif" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

6. Run the sink workflow below on a separate EVK and listen for the repeating sample through the sink speaker. No pairing, user-button action, or UART playback command is needed on the source. Audio comes from built-in assets, not a microphone, USB audio, or a host file

### Run the broadcast sink

During DevCon 2026, use the broadcast source provided by the presenter. When reproducing the lab afterward, start the source on a second EVK using the preceding workflow before verifying sink playback.

1. Follow [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox), select **KIT_PSE84_EVAL_EPC2**, and click **Browse for Application**. Select `<Lab source>/lab2_ble_audio/psoc-edge-btstack-broadcast-sink`, then select its checkbox and click **Create**
   <img src="assets/images/image_lab2_sink_import.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />
2. Once the project is imported, open the **Library Manager** by clicking **Configure Middleware** in the **ModusToolbox™ for VS Code** extension
3. Click **Add Library**, select **proj_cm55** as the **Target Project**, enable **mtb-pack-audio-sw-codecs** under **Middleware**, and then click **OK**

> **Note:** The ModusToolbox™ Audio SW Codecs Tech Pack needs to be installed as a separate package, as mentioned in the [Required development tools and prerequisites](#required-development-tools-and-prerequisites) section. Without the Tech Pack and `ENABLE_LC3_DECODING=1`, the sink can receive data but will remain silent.

> **Note:** Do not confuse the `mtb-pack-audio-sw-codecs` with `audio-sw-codecs`. The latter will be added as a dependency automatically, but this example requires both.

<img src="assets/images/image_lab2_addlibrary.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

4. Click **Update** in the Library Manager and wait for completion
5. Open `common.mk`, set `ENABLE_LC3_DECODING=1`, and confirm `AUDIO_CONFIG=MONO` to match the source used for this lab. If using `AUDIO_CONFIG=STEREO` afterward, set it on both boards and clean, rebuild, and reprogram both applications
```diff
###############################################################################
# Application-specific settings
###############################################################################

# Enable/disable LC3 decoding functionality. Options include:
#
# 0 -- Disable LC3 decoding (Default) - Works without any additional packs
# 1 -- Enable LC3 decoding - Requires ModusToolbox Audio SW Codecs Tech Pack
#
-ENABLE_LC3_DECODING=0
+ENABLE_LC3_DECODING=1

# Default audio configuration. Options include:
#
# MONO   -- Decoding and playback of 1 channel audio. 2nd channel data from
#           Bluetooth is ignored. (Default)
# STEREO -- Decoding and playback of 2 channels audio.
AUDIO_CONFIG=MONO
```

6. Build and program the top-level application by clicking the **Program** button in the **ModusToolbox™ for VS Code** extension. This builds and programs all three projects

7. After programming completes, open the sink EVK's KitProg3 COM port in the serial terminal, configure the baud rate to 115200-8-N-1, and press the kit reset button.
   Confirm that `PSOC Edge MCU: Bluetooth LE Audio Broadcast Sink Example` appears. After Bluetooth® initialization, the terminal prints `Broadcast Sink initialized. Scanning for broadcast sources...` and the sink scans automatically:

<img src="assets/images/image_lab2_sink_output1.gif" alt="Figure" style="width:635px; max-width:100%; height:auto; display:block; margin:0 auto;" />

8. With the presenter's source broadcasting at DevCon 2026, or your second EVK broadcasting afterward, wait for the sink to discover and synchronize to the broadcast automatically. Look for `Broadcast audio synchronized.` followed by the broadcast ID, and verify that the repeating sample plays through the EVK speaker. This is broadcast reception, not a paired Bluetooth® connection

<img src="assets/images/image_lab2_sink_output2.gif" alt="Figure" style="width:635px; max-width:100%; height:auto; display:block; margin:0 auto;" />

### Adjust the default volume 

The function `audio_driver_set_volume` can be used to change the output volume of the application.

1. Open `proj_cm55/source/bluetooth/broadcast_sink/broadcast_sink_bis.c` and observe that the volume is set in the `broadcast_sink_bis_isoc_cb()` function:
```c
        printf("Audio configuration: %lu Hz, %lu us, %u bytes/frame, %u BIS\n",
               (unsigned long)p_csc->sampling_frequency,
               (unsigned long)p_csc->frame_duration,
               p_csc->octets_per_codec_frame,
               p_big_sync_established->num_bis);
        audio_driver_set_volume((DEFAULT_VOL * 100) / 255);
```

2. Adjust the parameter, or modify `DEFAULT_VOL`, as shown in the following example:
```diff
-#define DEFAULT_VOL 200   /* default volume 0-255 scale (~78%) */
+#define DEFAULT_VOL 100   /* default volume 0-255 scale (~78%) */
```

3. Rebuild the application and reprogram the device. 
   The application should run again with the updated volume

### Conclusion

After executing this lab, you demonstrated Bluetooth® Low Energy (BLE) Audio using the PSOC™ Edge E84 MCU as the Bluetooth® host and the AIROC™ CYW55513 Wi-Fi and Bluetooth® combo device as the Bluetooth® controller. You created, built, programmed, and verified the broadcast sink, playing audio through the EVK speaker from the presenter's broadcast source at DevCon 2026. The included broadcast source workflow lets you reproduce both sides of the audio link on two EVKs afterward. Through this exercise, you gained hands-on experience with the BLE Audio software flow, the interaction between the host MCU and the wireless controller, and the ModusToolbox™ tools used to bring up a multi-core, connectivity-enabled application.

In addition, the sink uses Infineon’s audio codec library for LC3 decoding, while the source can transmit pre-encoded audio or encode the built-in recording with Google liblc3. 

The audio codec library can leverage Arm® Helium, also known as the M-Profile Vector Extension (MVE), on the Cortex-M55 CPU in PSOC™ Edge. Its 128-bit vector operations process multiple samples in parallel, accelerating suitable arithmetic kernels such as multiply-accumulate operations, filtering, and transforms. With a Helium-optimized codec build, less CPU time is needed to decode each audio frame, providing more margin to meet real-time playback deadlines and more headroom for additional audio channels, noise suppression, or machine learning. Helium executes within the CPU; it is not a separate audio accelerator. Faster processing can also enable longer idle periods or a lower CPU clock, potentially reducing energy consumption when the rest of the system permits it.

BLE Audio's lower power consumption compared to Bluetooth® Classic audio translates directly into longer battery life, or a smaller battery, for earbuds, hearing aids, and other wearable audio products, an important differentiator in a competitive consumer market. Accelerating codec processing with Helium also frees CPU headroom for value-added features such as voice assistants or connectivity housekeeping, without requiring a more expensive, higher-power processor.

Together, these outcomes provide a solid foundation for extending your design with more advanced BLE Audio features and richer audio processing in your applications.

## Lab 3: Adding connectivity to machine learning with dual-core virtual MQTT client

### Objective

In this lab, you will learn how to add MQTT functionality to a machine learning application running on PSOC™ Edge, and how to leverage the Virtual Connectivity Manager (VCM) to easily enable multi-core support through virtualization.

### Description

This hands-on lab shows how to build a complete IoT pipeline on an Infineon PSOC™ Edge dual-core MCU. You’ll start from the DEEPCRAFT™ Deploy Motion classification example to run an on-device model, then add **Wi-Fi** and **MQTT** connectivity on the CM33 leveraging the Wi-Fi MQTT Client example. Finally, you’ll introduce Infineon’s **Virtual Connectivity Manager (VCM)** so the AI core (CM55) can publish MQTT messages while the CM33 securely maintains the network stack and broker connection.

**MQTT** is a lightweight publish/subscribe messaging protocol for IoT. Devices (clients) publish messages to topics on a broker; subscribers receive messages for topics they’re interested in. It’s efficient over low-bandwidth links and supports QoS and keep-alive, making it ideal for telemetry like motion classification results.

**VCM** is a middleware layer that virtualizes connectivity across cores. The CM33 owns the Wi-Fi/TCP/IP/TLS/MQTT stack, while other cores (e.g., CM55) call “virtual” connectivity APIs that VCM transparently proxies over inter-processor communication. This keeps the network stack single-owned and thread-safe, while letting the ML core publish as if it had native access.

**Why ModusToolbox™ for ML + connectivity**:

- Unified, curated middleware (Wi-Fi, MQTT, TLS, RTOS, sensors) with the Library Manager for fast integration
- Board Support Packages and dual-core workflows that simplify bring-up and partitioning
- Consistent tooling for device configuration, security assets, and production scaling

Why **VCM** in a multi-core system:

- Clean separation of concerns: CM55 focuses on inference, CM33 on connectivity
- Single network stack ownership avoids conflicts and reduces memory footprint on the ML core
- Portable, scalable design: additional cores can reuse connectivity without rewriting application logic

By the end of this hands-on lab, you’ll have a machine learning motion classifier publishing results to the cloud through a robust, production-ready, multi-core architecture.

### Hardware diagram

This lab can use the PSOC™ Edge E84 EVK or the PSOC™ Edge E84 AI Kit.

The main hardware components utilized by this lab are:

- UART: output information to serial terminal
- SDIO: Wi-Fi communication between PSOC™ Edge and CYW55513
- `USER_LED1`: red LED displaying activity
- `USER_BTN1`: button for MQTT Client demo
- I2C: communication with BMI270 6-axis IMU with accelerometer and gyroscope

Below are the hardware block diagrams for this example using the PSOC™ Edge E84 EVK (top) and PSOC™ Edge E84 AI Kit:

<img src="assets/images/image_lab3_evk.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

<img src="assets/images/image_lab3_ai.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

> **Note:** For the KIT_PSE84_EVAL_EPC2 EVK, set BOOT SW to ON. See [Appendix B: KIT_PSE84_EVAL details](#appendix-b-kit_pse84_eval-details).

### Project creation

1. Follow the steps in [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox) to create a new application.
   When creating the application, select the **PSOC™ Edge Machine Learning DEEPCRAFT™ Deploy Motion** application under the **Machine Learning** section.
   The *DEEPCRAFT™ Deploy Motion* application demonstrates how to deploy an ML model generated using *DEEPCRAFT™ Studio*. This model was trained to detect motion. This lab is not intended to explain DEEPCRAFT™ Studio or machine learning in detail; it focuses on how to add MQTT functionality to an ML demo. To learn more about machine learning solutions, visit the [PSOC™ Edge Training Collection](https://infineon-academy.csod.com/ui/lms-learner-playlist/PlaylistDetails?playlistId=8f04565f-88f4-4ca7-83b3-22e501656fbd)

> **Note:** ModusToolbox™ also includes the [mtb-example-psoc-edge-wifi-dual-core-virtual-mqtt-client](https://github.com/Infineon/mtb-example-psoc-edge-wifi-dual-core-virtual-mqtt-client) code example showing how to implement a dual-core MQTT client. This example will be used as a base for this lab; however, the modified source has been included in this training for convenience.

> **Warning:** Windows has a 260-character path length limit. A long workspace path and/or a long project name may cause build issues when creating some applications.

<img src="assets/images/image_lab3_deepcraft_motion_template.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. After importing the project to Visual Studio Code, build and program the machine learning Deploy Motion application by clicking the **Program** button in the **ModusToolbox™ for VS Code** extension

<img src="assets/images/image_lab3_program_motion_application.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. After the program runs successfully, open the serial terminal and configure the baud rate to 115200-8-N-1. The application will output the score for each motion type: unlabeled, circle, or shaking.
   While holding the kit, shake the board or move it in a circling motion and observe the results in the terminal

<img src="assets/images/image_lab3_output1.gif" alt="Figure" style="width:467px; max-width:100%; height:auto; display:block; margin:0 auto;" />

You have seen how easy it is to run a machine learning demo on PSOC™ Edge. Feel free to visit the [DEEPCRAFT™ AI Hub](https://www.infineon.com/psocdeveloper/ai-hub) for more information about other AI solutions from Infineon.

### Testing MQTT functionality with VCM

The machine learning model used in the previous steps is running on the CM55 CPU and the U55 Ethos NPU, while the CM33 CPU is only taking care of initialization, and then stays idle in Deep Sleep. 
In this section, you will test the dual-core MQTT functionality: the CM33 CPU handles the Wi-Fi and MQTT connection, while the CM55 CPU publishes MQTT messages to the cloud.

#### Optional: Run a local Mosquitto MQTT broker (not required for DevCon)

These steps are only needed when replicating the MQTT test on a local bench. They are **not required for the live training at DevCon**; use the broker details and certificates provided by the presenter during the training.

1. Download and install Mosquitto from [mosquitto.org/download](https://mosquitto.org/download). During setup, do not install Mosquitto as a Windows service. Add the Mosquitto installation directory to the system `PATH`

2. In the Windows Start menu, open **modus-shell**, and then navigate to the following location in the Lab Source folder included with this training:
    `Lab_Source/lab3_ml_vcm/mosquitto_scripts`

3. Generate TLS certificates using the PC's IPv4 address. Find the address with `ipconfig`; use the address reachable by the evaluation kit, not `localhost`
```bash
   sh generate_ssl_cert.sh <PC-LAN-IPv4-address>
```

<img src="assets/images/image_lab3_mosquitto_1.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

4. Format the generated CA certificate, client certificate, and client key so they can be added to the C configuration:
```bash
   python format.py mosquitto_client.crt mosquitto_client.key mosquitto_ca.crt
```

   **Note:** The certificates will be saved to local files. The generated strings will be used in the next sections when configuring the client. 

<img src="assets/images/image_lab3_mosquitto_2.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

5. Open `mosquitto.conf` in the same `mosquitto_scripts` folder and adjust the port if needed. The port should not be restricted by your operating system firewall.
   In this case, use port **50007**, which you will configure on the client in the next section

<img src="assets/images/image_lab3_mosquitto_3.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

6. Create usernames and passwords for the MQTT broker using the following format:
```bash
   mosquitto_passwd -c <passwordfile> <username>
```

   **Note:** A script `create_pwfile_users.py` is included in the `mosquitto_scripts` folder to register multiple users. 

7. In the `mosquitto_scripts` directory, start Mosquitto with the example configuration:
```bash
   mosquitto -v -c mosquitto.conf
```

   Leave this terminal open while testing MQTT. Ensure the PC and evaluation kit are connected to the same network.

<img src="assets/images/image_lab3_mosquitto_4.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

8. [INFO] Tools such as MQTT Explorer can be used to observe MQTT messages in a graphical interface.   
   The presenter will be running MQTT Explorer to show messages during the session

<img src="assets/images/image_lab3_mqtt_explorer.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

#### Adding MQTT broker support to the DEEPCRAFT™ Deploy Motion example

The following steps explain how to add Wi-Fi and MQTT support to the previously created DEEPCRAFT™ Deploy Motion example.
ModusToolbox™ includes the **[mtb-example-psoc-edge-wifi-dual-core-virtual-mqtt-client](https://github.com/Infineon/mtb-example-psoc-edge-wifi-dual-core-virtual-mqtt-client)** code example showing how to implement a dual-core MQTT client. This example will be used as a base for this lab; however, the example utilizes the CM55 core to handle the Wi-Fi and MQTT connection, while in this lab, the CM33 core is used for that purpose. It is recommended that you visit and test the code example at your own pace; however, this lab includes modified source for your convenience.

1. Open the previously created **PSOC™ Edge Machine Learning DEEPCRAFT™ Deploy Motion** code example in Visual Studio Code
2. Click the **Configure Middleware** button in the **ModusToolbox™ for VS Code** extension to open the Library Manager

<img src="assets/images/image_lab3_libmgr_1.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. Click the **Add Library** button and add the following libraries for both the **proj_cm33_ns** and **proj_cm55** projects. Click **Update**, wait for the libraries to be updated in the project, and then close the **Library Manager**:

    - freertos
    - mqtt
    - virtual-connectivity-manager
    - wifi-core-freertos-lwip-mbedtls

4. Copy the following folder from the Lab Source folder included with this training to your project:
   `Lab_Source/lab3_ml_vcm/mod1-add-mqtt-vcm`

   Files from `mod1-add-mqtt-vcm` should be added or overwrite existing files, but other files should remain untouched in your VS Code project. You can use the following command in modus-shell:
```bash
   cp -rf <Lab_Source>/lab3_ml_vcm/mod1-add-mqtt-vcm/* <path to your project>
```

<img src="assets/images/image_lab3_add_mqtt.png" alt="Figure" style="width:400px; max-width:100%; height:auto; display:block; margin:0 auto;" />

5. Open `mtb_shared/wifi-host-driver/release-<version>/generated_mac_address.txt` and `NVRAM_GENERATED_MAC_ADDRESS` to assign a MAC address

> **Note:** This step is optional when testing one device, but it is necessary when connecting multiple EVKs to the same router. CYW55513 contains OTP bits which are programmed during production by the OEM or module partner. The OTP bits are not programmed in the EVK, so the device reads the MAC address from the file above, or you can program the OTP bits using APIs.

6. Open `proj_cm33_ns/configs/wifi_config.h` and modify the Wi-Fi settings. The following table shows the values for the DevCon 2026 event

| Macro                | DevCon 2026 demo               |
| -------------------- | ------------------------------ |
| `WIFI_SSID`          | GL-PSOCEdge                    |
| `WIFI_PASSWORD`      | PSE84WiFi                      |
| `WIFI_SECURITY_TYPE` | `CY_WCM_SECURITY_WPA2_AES_PSK` |

7. Open `proj_cm33_ns/configs/mqtt_client_config.h` and modify the following MQTT parameters:

| Macro                    | DevCon 2026 demo                                 |
| ------------------------ | ------------------------------------------------ |
| `MQTT_BROKER_ADDRESS`    | “192.168.8.213”                                  |
| `MQTT_PORT`              | 50007                                            |
| `MQTT_SECURE_CONNECTION` | ( 1 )                                            |
| `MQTT_USERNAME`          | Check the username with the presenter (e.g., “user1”)     |
| `MQTT_PASSWORD`          | “pwd”                                            |
| `MQTT_PUB_TOPIC`         | Use your own topic (e.g., “`ledstatus_yourname`”) |
| `MQTT_SUB_TOPIC`         | Use your own topic (e.g., “`ledstatus_yourname`”) |

In the same `proj_cm33_ns/mqtt_client_config.h` file, copy the following certificates to the corresponding macros:

> **Note:** These certificates were created for training purposes because the presenter runs the MQTT broker. It is not necessary to create your own certificates for this training; however, you can create certificates by following the instructions in [Optional: Run a local Mosquitto MQTT broker](#optional-run-a-local-mosquitto-mqtt-broker-not-required-for-devcon).

```c
#define CLIENT_CERTIFICATE \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDYTCCAkkCFENMHBIB1iyLJ6fEvC6CenvSEglvMA0GCSqGSIb3DQEBCwUAMGcx\n"\
"CzAJBgNVBAYTAklOMRIwEAYDVQQIDAlLYXJuYXRha2ExEjAQBgNVBAcMCUJlbmdh\n"\
"bHVydTELMAkGA1UECgwCQ1kxFDASBgNVBAsMC0VuZ2luZWVyaW5nMQ0wCwYDVQQD\n"\
"DARteUNBMB4XDTI2MTAwMjIxNDg1NloXDTM2MDkyOTIxNDg1NlowczELMAkGA1UE\n"\
"BhMCSU4xEjAQBgNVBAgMCUthcm5hdGFrYTESMBAGA1UEBwwJQmVuZ2FsdXJ1MQsw\n"\
"CQYDVQQKDAJDWTEUMBIGA1UECwwLRW5naW5lZXJpbmcxGTAXBgNVBAMMEG1vc3F1\n"\
"aXR0b19jbGllbnQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2UmRQ\n"\
"kZ37uZ2H7Xg9o8YarXl1ODufGUURyrvyhE8DZWwDQY/iVBVyGWJRVcaHeZiI+tsI\n"\
"lLhWmpcBVh+7bjESnV7Edpm27/Lj/7/dVZWhJUNNwRiJIAbx0ogmtXp6EKSZ5vym\n"\
"ltmSqzCJ980q2H+UoifaI/xkPt5ZJr1QocpzL2YLWC4J0RBlZkWVOcaBDdMgOfTz\n"\
"4U397zLbWva1dI3g5b7Hk8LC6OR0EtqCexfGFUf8sYWcI5gcvx2LsmBBS7y/UEBH\n"\
"tV2pYPdIP1/u0Y5nDxzLHfKet1ocG/AFwef+AYmL1sKKzEt0SchiiYGmgHfoEGsR\n"\
"1HmOuB6s9UVBId9vAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAJugA+Gta1/bWbOV\n"\
"bK2ZPPtfMgV1fUCpr7jbk4kWLddjsBNZ6fWlNtdracierE8FUZxv2he89efUp3NS\n"\
"N0FZPqwMTCs+f+Tey0PDKlZrReE8hwNzNJR7vdVXWhVjYVwbcLKmGVnh6zayhBm+\n"\
"EI0UxvzcxR56/JMS+3RKbhmNI7CxCFkFbQoEH+qK9lWP/QQfo1Xks/8OpmMjZGqL\n"\
"x9UgcjWwIJH8LStIVzvxfAPegHnm3u/E6ZkeXqZ5Kd6odvQZjnbNeV9QI7zrMzi0\n"\
"7fqMT67i3Ms9Hzr9sL/H2vrPCsdeHHHsS3Yo/OSHUqUEItKgkjGBCGLhY1JXBNIX\n"\
"bde8Dt8=\n"\
"-----END CERTIFICATE-----\n"
```
```c
#define CLIENT_PRIVATE_KEY \
"-----BEGIN PRIVATE KEY-----\n"\
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC2UmRQkZ37uZ2H\n"\
"7Xg9o8YarXl1ODufGUURyrvyhE8DZWwDQY/iVBVyGWJRVcaHeZiI+tsIlLhWmpcB\n"\
"Vh+7bjESnV7Edpm27/Lj/7/dVZWhJUNNwRiJIAbx0ogmtXp6EKSZ5vymltmSqzCJ\n"\
"980q2H+UoifaI/xkPt5ZJr1QocpzL2YLWC4J0RBlZkWVOcaBDdMgOfTz4U397zLb\n"\
"Wva1dI3g5b7Hk8LC6OR0EtqCexfGFUf8sYWcI5gcvx2LsmBBS7y/UEBHtV2pYPdI\n"\
"P1/u0Y5nDxzLHfKet1ocG/AFwef+AYmL1sKKzEt0SchiiYGmgHfoEGsR1HmOuB6s\n"\
"9UVBId9vAgMBAAECggEAWBH1riuhJmszqujtl8zoUZOxo4t91W0l/aGyZ0Q9TLUt\n"\
"12bQo7IdR+f2I7bs9x0oLycKLht07jSvs/AP1QC2CLlnAT0PJJzE9hjg7AA/DsAK\n"\
"wmD/wqFraV3a8ePhHVyzvjojmi8tO1mhUUwX2dYJztkDqi6O6TerPWJmua/ltPyD\n"\
"BF+O+WeU/mV4/pH4DNJ/vfghDcWjNwnquc0kb2qKa336ZQSVLFCdR+LMWg85rptr\n"\
"6YimoDArlYO/1ZAWDIY0wHWjXOZe9OErCwuXNbg+jdn8QdJjcmpyjda9wBNenuAs\n"\
"df8RcH1kylx+Q/QsKfAorpAv118fjNMnn0AW1Hb9uQKBgQDwWmF5n1V33Iq2Hjgn\n"\
"KVFpVXL56lRkxE7E3EXZDCuSiRtNU/gYjDLXQF9xQmgk8wsWrngfMXAWFOQQQgTa\n"\
"mORUu4nnNwLCpcUbes0KJ1o8WSRvxLI7+lKVhIrIunc4ffsLPzZtQZM6EuHq7f9C\n"\
"5yrzbjQ6Vknhd04VEFfUldG1pwKBgQDCMONmOz6oCm3CD+QW4TsxlS4+5XqUNKod\n"\
"f0BYO0/t8oHvnz5HpL4VtHhuESWqvPiE7+HH7w86OYMyYNem8fo0CogxK3+oSUDU\n"\
"NBeZDX757SVIQ6Buea5qnFXvWUnhHPd+fv4/MGYH6LlJmu5x0jezjuPn3DL/Rroa\n"\
"WQBrOfxQ+QKBgQDPMwsO3vW+I6iMwVZlJDBjnt9EZOcmCzlgagfoyZ4ScBHSQs4A\n"\
"03PMrljY+YdwOvlXL0aslWDsGExXW6J1lBJanWWPppPBm0hlnSJ/W1dl6O8JT0bb\n"\
"f7uL27wMuPqn/6rYkkDoRPyXtsl9Tnicg046lsl9dP+x17i/XdxpjlI/xwKBgGlS\n"\
"jLNQ5K3NYjRD3CjQpgNBbyCr4+zoF3ACKYrxOGvNAM5PJz9CSdqJ1FuWL0DIV136\n"\
"oRGIRlEFCnRTdANW8KYzJCTO++DxQhkV28qmOD0jcvobu7LPilrGShGT8u8Gf/F6\n"\
"vTjWbjBR99TFFBhltNJNaKzDkGFGIf/ST9jYTVI5AoGBAM56Qq8f/gGxh4+wQ4HQ\n"\
"hlua75zpqpAOkeC11X7kMgexWOWwPUyR1dh8MJGPxCz69+gb/BaVE5EOYgB0jySi\n"\
"8jVmy3KYeIaeR9bgYWcgHVrvB4BlmTYE8tR0FOnsSOLbNH5zX4EehbRc67H4oSYY\n"\
"JrzrJfbEfSL+jHlU8K+2HL7O\n"\
"-----END PRIVATE KEY-----\n"
```
```c
#define ROOT_CA_CERTIFICATE \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDrzCCApegAwIBAgIUcX4wui1JiAYhvFCjeucFygRIm9EwDQYJKoZIhvcNAQEL\n"\
"BQAwZzELMAkGA1UEBhMCSU4xEjAQBgNVBAgMCUthcm5hdGFrYTESMBAGA1UEBwwJ\n"\
"QmVuZ2FsdXJ1MQswCQYDVQQKDAJDWTEUMBIGA1UECwwLRW5naW5lZXJpbmcxDTAL\n"\
"BgNVBAMMBG15Q0EwHhcNMjYxMDAyMjE0ODUyWhcNMzYwOTI5MjE0ODUyWjBnMQsw\n"\
"CQYDVQQGEwJJTjESMBAGA1UECAwJS2FybmF0YWthMRIwEAYDVQQHDAlCZW5nYWx1\n"\
"cnUxCzAJBgNVBAoMAkNZMRQwEgYDVQQLDAtFbmdpbmVlcmluZzENMAsGA1UEAwwE\n"\
"bXlDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAOFl1F4kJcLcrbYn\n"\
"mA775zElKAxdaEGu0Zf/5UR711CSrabV6U9c5FkehEYCKJDgs+ktKowqyDCE4bJq\n"\
"G1WKiY8rg7zKoCR7G1B+mfIFvdHVQW1CciNDQm/R6GTS7EEfaLO3xgeU8naae+Ad\n"\
"EtMT0EXrD3BXMPFfqnNWqznhUPH4tANlVLPZs4DJuPVCd5QQ4HrI3mtmAXClmzFs\n"\
"zGGaZ07qGPTx87kJHLAJLb1YYjfdFGQtida0JRHVNECPihxzAjB+fMdE3xj3mah1\n"\
"oJw5jHhb4Pm/k8xV7aZSo4JeCIX+ZMzOp0vDq1LUtdoJ7C8p1+P55cnkf1YX9yRf\n"\
"Igw3/XcCAwEAAaNTMFEwHQYDVR0OBBYEFIsFlffM9NOa1XTNn+Rh6cTLdsGeMB8G\n"\
"A1UdIwQYMBaAFIsFlffM9NOa1XTNn+Rh6cTLdsGeMA8GA1UdEwEB/wQFMAMBAf8w\n"\
"DQYJKoZIhvcNAQELBQADggEBALWbZYwZsmKhdqyeAzPM0ScaA3ZBueT4RDpytqcF\n"\
"8FK5X7HAh+kfaifwiboNZTzy09kZAQXfvJaMVMuV/dkVYsmkgAAWgiIsOzLQ/eYn\n"\
"qSJgEDjuNVnKuFOOR2x4g2GC8T1aIi5HKYxj+RUcoRuGxQPYq7831paa2XZUPafh\n"\
"q1Q5ossPshSCAe304X8E7COIs8N9SHx4BG+gaop/uq7+0gYQeNbnADI2vuB8cks4\n"\
"if7OeeTG8UXsfgZCIqbuQIrdxVtP4HZDfuf7aMeELCRK3L/hgLZ6dGOro+Dj9hjM\n"\
"LULMWRYfbrfywzLrvJgcQb5sbTLSjmY92cma6jK5YpUYYA0=\n"\
"-----END CERTIFICATE-----\n"
```

8. Open `proj_cm33_ns/main.c` and look for `TODO 1`.
   Call `cy_vcm_init` to initialize VCM on CM33 before booting the CM55 CPU. Observe that the CM33 acts as the primary core, so it creates the resource, and both CPUs use Inter-Processor Communication (IPC) to exchange packets
```c
    /* TODO 1: Initialize VCM before booting CM55. CM33 boots first, so it
     * creates the IPC resources that CM55 uses.
     *  - Fill in primary_core_config: ipc_obj = &cybsp_cm33_ipc_instance,
     *    hal_resource_opt = CY_VCM_CREATE_HAL_RESOURCE,
     *    channel_num = MTB_IPC_CHAN_1, event_cb = vcm_callback.
     *  - Call cy_vcm_init() and call handle_app_error() if it fails.
     */
    primary_core_config.ipc_obj = &cybsp_cm33_ipc_instance;
    primary_core_config.hal_resource_opt = CY_VCM_CREATE_HAL_RESOURCE;
    primary_core_config.channel_num = MTB_IPC_CHAN_1;
    primary_core_config.event_cb = vcm_callback;
    result = cy_vcm_init(&primary_core_config);

    /* VCM Initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable CM55. */
    /* CY_CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);
```

9. Open `proj_cm55/source/virtual_mqtt_task.c` and look for `TODO 2` and `TODO 3`.
   Initialize VCM for the CM55, which acts as the secondary core, so it uses the resource created by the CM33 CPU
```c
    /* TODO 2: Initialize VCM on CM55. CM33 created the IPC resources before
     * booting CM55, so CM55 uses them.
     *  - Fill in secondary_core_config: ipc_obj = &cybsp_cm55_ipc_instance,
     *    hal_resource_opt = CY_VCM_USE_HAL_RESOURCE,
     *    channel_num = MTB_IPC_CHAN_1, event_cb = vcm_callback.
     *  - Call cy_vcm_init() and store the return value in 'result' (replace
     *    the placeholder below).
     */
    secondary_core_config.ipc_obj = &cybsp_cm55_ipc_instance;
    secondary_core_config.hal_resource_opt = CY_VCM_USE_HAL_RESOURCE;
    secondary_core_config.channel_num = MTB_IPC_CHAN_1;
    secondary_core_config.event_cb = vcm_callback;
    result = cy_vcm_init(&secondary_core_config);

    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("\nVCM Initialization failed on CM55.\r\n");
        vTaskDelete(NULL);
    }
    safe_print("\nVirtual Connectivity Manager Initialized on CM55\r\n");

    /* TODO 3: Wait until CM33 signals that the MQTT connection is ready:
     * while Cy_IPC_Sema_Status(MQTT_READY_SEMA_NUM) returns
     * CY_IPC_SEMA_STATUS_UNLOCKED, call vTaskDelay(pdMS_TO_TICKS(WAIT_DELAY_MS)).
     */
    safe_print("\nPlease wait until MQTT is connected on CM33...\r\n");
    while (CY_IPC_SEMA_STATUS_UNLOCKED == Cy_IPC_Sema_Status(MQTT_READY_SEMA_NUM))
    {
        vTaskDelay(pdMS_TO_TICKS(WAIT_DELAY_MS));
    }
```

10. Build and program the updated project.
   The terminal will continue showing the results of the machine learning motion detection model, but it will also show the information about the Wi-Fi connection and then the connection to the MQTT broker.
   Once connected, press **USER_BTN1** to toggle the status of the LED and publish it to the MQTT broker

   > **Note:** The MQTT broker must be running as described in [Optional: Run a local Mosquitto broker](#optional-run-a-local-mosquitto-mqtt-broker-not-required-for-devcon). An MQTT viewer, such as MQTT Explorer, can be used to observe and send messages.

<img src="assets/images/image_lab3_mod1_results.gif" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

### Publish the machine learning results via MQTT

You have now combined the functionality of **PSOC™ Edge Machine Learning DEEPCRAFT™ Deploy Motion** and a modified version of **PSOC™ Edge MCU: Dual-core virtual MQTT client** to use the CM33 CPU to handle the Wi-Fi and MQTT connection, while the CM55 runs machine learning inferences. 
Now, modify the example to send machine learning results using the virtual MQTT connection.

1. Open `proj_cm55/source/publisher_task.h` and add the following code to define a new command and function to publish the inference results

    * Modification #1, add `PUBLISH_INFERENCE_RESULT` definition:
```c
/* Commands for the Publisher Task. */
typedef enum
{
    PUBLISH_MQTT_MSG,
    PUBLISH_INFERENCE_RESULT 
} publisher_cmd_t;
```

* Modification #2, add the prototype for `publisher_publish_inference_result`:
```c
/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void publisher_task(void *pvParameters);
void publisher_button_interrupt_handler(void);
+BaseType_t publisher_publish_inference_result(const char *status);
```

2. Open `proj_cm55/source/publisher_task.c` and add the following code to handle the new task to publish the inference results.
   Customize your topic as needed

    * Modification #1, add definition for `INFERENCE_RESULT_TOPIC`:
```c
/*******************************************************************************
* Macros
*******************************************************************************/
/* Queue length of a message queue that is used to communicate with the
 * publisher task.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH     (3U)

/* Publisher topic used for secondary core (CM55). CM33 subscribes to it. */
#define SECONDARY_PUB_TOPIC             "GREEN_APP_STATUS" /* Customize your topic */
#define INFERENCE_RESULT_TOPIC          "ML_INFERENCE_RESULT" /* Customize your topic */

/* Button presses closer than this are treated as switch bounce. */
#define DEBOUNCE_TIME_MS                (200U)
```

   * Modification #2, define global variables for `inference_publish_info`:
```c
/*******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT publish settings for inference status updates. */
static cy_mqtt_publish_info_t inference_publish_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = INFERENCE_RESULT_TOPIC,
    .topic_len = (sizeof(INFERENCE_RESULT_TOPIC) - 1),
    .retain = false,
    .dup = false
};

/* FreeRTOS task handle for this task. */
```

   * Modification #3, define `publisher_publish_inference_result` function:
```c
/* Queue a changed inference status for the publisher task. */
BaseType_t publisher_publish_inference_result(const char *status)
{
    publisher_data_t publisher_q_data;

    if ((NULL == publisher_task_q) || (NULL == status))
    {
        return pdFALSE;
    }

    publisher_q_data.cmd = PUBLISH_INFERENCE_RESULT;
    publisher_q_data.data = (char *)status;
    return xQueueSend(publisher_task_q, &publisher_q_data, 0U);
}

/*******************************************************************************
* Function Name: publisher_task
********************************************************************************
```

   * Modification #4, modify `publisher_task` to handle both MQTT topics:
```c
void publisher_task(void *pvParameters)
{
    /* Status variable */
    cy_rslt_t result;

    publisher_data_t publisher_q_data;

    /* Command to the Virtual MQTT task */
    virtual_mqtt_task_cmd_t virtual_mqtt_task_cmd;
    cy_mqtt_publish_info_t *publish_info;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Create a message queue to communicate with the button interrupt. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

    safe_print("\nPress the user button (SW1) to publish \"%s\"/\"%s\" on the topic '%s'...\r\n",
               ON_MESSAGE, OFF_MESSAGE, secondary_publish_info.topic);

    while (true)
    {
        /* Wait for commands from the button interrupt. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data.cmd)
            {
                case PUBLISH_MQTT_MSG:
                {
                    publish_info = &secondary_publish_info;
                    break;
                }

                case PUBLISH_INFERENCE_RESULT:
                {
                    publish_info = &inference_publish_info;
                    break;
                }

                default:
                    continue;
            }

            publish_info->payload = publisher_q_data.data;
            publish_info->payload_len = strlen(publish_info->payload);

            safe_print("\nPublisher(m55): Publishing '%s' on the topic '%s'\r\n",
                       (char *)publish_info->payload, publish_info->topic);

            result = cy_mqtt_publish(virtual_mqtt_connection, publish_info);
            if (CY_RSLT_SUCCESS != result)
            {
                safe_print("  Publisher: MQTT Publish failed with error 0x%0X.\r\n",
                           (int)result);

                /* Communicate the publish failure with the Virtual MQTT task. */
                virtual_mqtt_task_cmd = HANDLE_VIRTUAL_MQTT_PUBLISH_FAILURE;
                xQueueSend(virtual_mqtt_task_data_q, &virtual_mqtt_task_cmd, portMAX_DELAY);
            }
        }
    }
}
```

3. Lastly, perform the following modifications to `imu_data_process()` in `proj_cm55/imu.c` to publish the inference results when a change is detected

    * Modification #1, add local static variable `last_queued_inference_label`:
```diff
cy_rslt_t imu_data_process(void)
{
    /* Variables required to get the IMU data and process the results. */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    float label_scores[IMAI_DATA_OUT_COUNT];
    char *label_text[] = IMAI_DATA_OUT_SYMBOLS;
    int16_t best_label;
    float max_score;
    uint16_t fifo_length = 0;
#ifdef IMU_ACC
    uint16_t accel_frame_length;
#endif /* IMU_ACC */
#ifdef IMU_GYR
    uint16_t gyro_frame_length;
#endif /* IMU_GYR */
    uint16_t int_status = 0;
    uint16_t watermark = 0;
    uint16_t index = 0;
+    static int16_t last_queued_inference_label = -1;
```

   * Modification #2, publish the result when the inference result changes:
```diff
             /* Check if there is any model output */
            switch(IMAI_dequeue(label_scores))
            {
                /* On success, print the labels with their scores */
                case IMAI_RET_SUCCESS:
                {
                    for(int i = 0; i < IMAI_DATA_OUT_COUNT; i++)
                    {
                        printf("label: %-10s: score: %f\r\n", label_text[i],
                               label_scores[i]);
                        if (label_scores[i] > max_score)
                        {
                            max_score = label_scores[i];
                            best_label = i;
                        }
                    }

+                    if (best_label != last_queued_inference_label)
+                    {
+                        if (pdPASS == publisher_publish_inference_result(label_text[best_label]))
+                        {
+                            last_queued_inference_label = best_label;
+                        }
+                    }

                    printf("\r\n");
                    printf("Output: %-30s\r\n", label_text[best_label]);
                    break;
                }   
```

4. Rebuild and reprogram the device. You should observe that the inference results are now published to the broker together with the LED status

<img src="assets/images/image_lab3_mod2_results.gif" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

### Conclusion

You successfully added MQTT support to a DEEPCRAFT™ machine learning example using PSOC™ Edge. You integrated VCM, enabling the CM55 to focus on machine learning tasks and to publish messages via a virtual connection while the CM33 application manages the Wi-Fi connection.

This demonstrates not only how ModusToolbox™ facilitates easy middleware integration, but also the value of VCM in abstracting the IPC implementation—making multi-core connectivity application development simpler.

For teams building ML-enabled products, this architecture lets the machine learning workload and the network stack be developed, updated, and validated independently, without changes on one core risking the stability of the other. A single, well-tested connectivity implementation on the CM33 can also serve multiple ML applications on the CM55 over time, avoiding the need to re-validate Wi-Fi and cloud connectivity every time the ML model changes, which shortens the path from prototype to a certified product for condition monitoring, predictive maintenance, or smart-sensor applications.

## Lab 4: WLAN Low-Power Assistant

### Objective

This lab demonstrates the low-power operation of a host PSOC™ Edge E84 MCU and a WLAN AIROC™ CYW55513 Wi-Fi & Bluetooth® combo chip using the network activity handlers provided by the low-power assistant (LPA) middleware.

### Description

The LPA middleware for Wi-Fi provides a straightforward way to make the low-power features available to developers in the form of a portable configuration layer. It provides features implementing low-power functionality for MCUs, Wi-Fi, and Bluetooth®.

The LPA middleware is essentially a Wi-Fi offload manager that is instantiated when Wi-Fi Connection Manager (WCM) initialization is done. The offload manager manages the offload configurations which are created using the Device Configurator.

LPA consists of the following components:

- A configurator tool (using a personality), which makes the low-power features of the system easy to use (see **ModusToolbox™ Device Configurator Tool Guide**). This personality writes data structures that are processed by the firmware and implement the choices made in the personality.
- LPA firmware that is used at system initialization and does not require user interaction.
- A small firmware module that provides integration between the low-power firmware and the user application. This final piece of firmware is part of the end-user application.

Some of the features supported by LPA include:

- **MCU low power:** Allows you to configure the MCU to enter a low-power mode to achieve maximum power savings.
- **Wi-Fi low power:** Reduces host MCU wakeups by allowing the WLAN device to handle routine network traffic and protocol maintenance while the MCU remains in a low-power state.
	- **Wi-Fi host-wake signal:** Allows the WLAN device to wake the host MCU from its low-power state when host processing is required.
	- **Wi-Fi Address Resolution Protocol (ARP) offload:** Reduces the time the host needs to remain awake because of ARP broadcast traffic by handling that traffic in the WLAN device.
	- **Wi-Fi packet filter offload:** Limits which packets the WLAN subsystem passes to the host processor. Filtering unwanted or unnecessary packets prevents them from waking the host from Deep Sleep or keeping it from entering Deep Sleep.
	- **Wi-Fi TCP keepalive offload:** Reduces host MCU power consumption by offloading TCP keepalive functionality to WLAN firmware.
	- **DHCP Lease Time Renew offload:** Reduces host MCU power consumption by offloading periodic DHCP requests for lease renewal to WLAN firmware.
	- **ICMP offload:** Reduces host MCU power consumption by allowing WLAN firmware to reply to ping requests from peers without waking the host.
	- **Neighbor Discovery offload:** Reduces host MCU power consumption by allowing WLAN firmware to send Neighbor Advertisement responses to Neighbor Solicitation requests from peers.
	- **NULL Keepalive offload:** Reduces host MCU power consumption by offloading the periodic transmission of NULL keepalive packets to WLAN firmware.
	- **NAT Keepalive offload:** Reduces host MCU power consumption by offloading the periodic transmission of NAT keepalive packets to WLAN firmware.
	- **Wake on Wireless LAN:** Allows the MCU to wake when received packets match configured patterns, avoiding unnecessary wakeups for unwanted packets.
	- **MQTT keepalive offload:** Reduces host MCU power consumption by offloading MQTT keepalive functionality to WLAN firmware.
- **Bluetooth® low power:** Enables Bluetooth® wake-up signaling by configuring the Bluetooth® host-wake and device-wake pins.

You will experiment with some of these features in this lab. They are explained in more detail in the LPA middleware library documentation and in the application note [AN241681: Low-power system design with PSOC™ Edge MCU and AIROC™ Wi-Fi & Bluetooth® combo chip](https://www.infineon.com/AN241681).

### Hardware diagram

The main hardware components utilized by this lab are:

- UART: output information to serial terminal
- SDIO: Wi-Fi communication between PSOC™ Edge and CYW55513
- `CYBSP_WIFI_HOST_WAKE`: Host wake-up signal from the CYW55513 to the host 

Below is the hardware block diagram for this example:

<img src="assets/images/image_lab4_evk.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

> **Note:** This example requires **BOOT SW** to be in the ON position.

See [Appendix B: KIT_PSE84_EVAL details](#appendix-b-kit_pse84_eval-details) for more details.

### Project creation

1. Follow the steps in [Appendix A: Creating a PSOC™ Edge application in ModusToolbox™](#appendix-a-creating-a-psoc-edge-application-in-modustoolbox) to create a new application.
   When creating the application, select the **PSOC™ Edge WLAN Offloads** application under the **Wi-Fi** section

> **Note:** This ModusToolbox™ example supports the PSOC™ Edge EVK. It is not currently available for the KIT_PSE84_AI AI Kit in ModusToolbox™.

> **Warning:** Windows has a 260-character path length limit. A long workspace path and/or a long project name may cause build issues when creating some applications.

<img src="assets/images/image_lab4_wlan_offloads_template.png" alt="Figure" style="width:637px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. Open `proj_cm33_ns/tcp_keepalive_offload.c` and modify the following parameters:

| Macro                | DevCon 2026 demo               |
| -------------------- | ------------------------------ |
| `WIFI_SSID`          | GL-PSOCEdge                    |
| `WIFI_PASSWORD`      | PSE84WiFi                      |
| `WIFI_SECURITY_TYPE` | `CY_WCM_SECURITY_WPA2_AES_PSK` |

3. Open `mtb_shared/wifi-host-driver/release-<version>/generated_mac_address.txt` and `NVRAM_GENERATED_MAC_ADDRESS` to assign a MAC address

> **Note:** This step is optional when testing one device, but it is necessary when connecting multiple EVKs to the same router. CYW55513 contains OTP bits which are programmed during production by the OEM or module partner. The OTP bits are not programmed in the EVK, so the device reads the MAC address from the file above, or you can program the OTP bits using APIs.

4. Build and program the **WLAN Offloads** application by clicking the **Program** button in the **ModusToolbox™ for VS Code** extension.
   Make sure the top level of the project is selected in the project workspace to ensure all three projects are built and programmed

5. After the program runs successfully, open the serial terminal and configure the baud rate to 115200-8-N-1. Then press the kit reset button. A message is printed as in the image below

> **Note:** Note the assigned IP address, because you will use it in the next steps.

<img src="assets/images/image_lab4_output1.gif" alt="Figure" style="width:659px; max-width:100%; height:auto; display:block; margin:0 auto;" />

6. Connect your PC to the same wireless network
7. Open a command prompt and use `nping` to send a TCP ping to the device’s IP address

> **Note:** `nping` is available with the Nmap installation described in the [Required development tools and prerequisites](#required-development-tools-and-prerequisites) section. The Nmap path (e.g., `C:\Program Files (x86)\Nmap`) should be added to the system path by the installer or manually.

```bash
nping --tcp <IP address>
```

8. Observe that the device not only responds to the ping request, but also wakes up, prints a message in the terminal, and then goes back to Deep Sleep mode

> **Note:** If the device does not respond, make sure both the PC and EVK are connected to the router and are in the same network.

<img src="assets/images/image_lab4_nping2.gif" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

> **Note:** The host MCU will wake up when any network activity is detected. The reasons for network activity can be due to the broadcast or multicast packets issued by the access point (AP). Further power saving can be implemented by using offload features like packet filtering, which will increase the time the host MCU will be in deep sleep.

9. Optionally, open Wireshark to observe the TCP packets

<img src="assets/images/image_lab4_nping_wireshark.gif" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

### Observe default project configuration

ModusToolbox™ includes different tools and configurators to aid in the configuration and development of your project. Two important tools enabling LPA are the **Device Configurator** and the **Library Manager**.

- The **Device Configurator** provides a graphical view of device peripherals and it generates macros, data structures, and initialization functions based on your selections. The BSP function `cybsp_init()` calls the generated functions to set up the clocks, pins, and internal routing. It is typically called from the `main()` function before using on-chip peripherals such as serial blocks and timer/counters.
- The **Library Manager** provides a GUI to select which Board Support Package (BSP) should be used when building a ModusToolbox™ application. The tool collects a list of available and currently selected BSPs and libraries, as well as all the necessary metadata from a web service. The tool allows you to add and remove BSPs and libraries, as well as change their versions.

Now, look at the default configuration of this project in the **Library Manager** and the **Device Configurator**.

1. [INFO] Open the **Library Manager** tool by clicking the **Configure Middleware** button in the **ModusToolbox™ for VS Code** extension

<img src="assets/images/image_lab4_open_library_manager.png" alt="Figure" style="width:600px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. [INFO] Observe that the Library Manager shows a list of libraries for each of the three projects (`cm33_s`, `cm33_ns`, and `cm55`).   
   The WLAN offloads project already includes the libraries needed to handle Wi-Fi and the low-power assistant library (LPA)

<img src="assets/images/image_lab4_lpa_dependencies.png" alt="Figure" style="width:734px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. [INFO] You can also observe that the LPA library is configured as “**Shared Git Repo**” and it is placed in the `mtb_shared` folder in your project.
   Feel free to check the library in the `mtb_shared` folder in ModusToolbox™.
   Close **Library Manager** after observing the libraries included in this project. You will not make any changes

> **Note:** Using `mtb_shared` is a way to share common libraries across a workspace; however, the Library Manager also allows you to make local copies.

<img src="assets/images/image_lab4_shared_library_location.png" alt="Figure" style="width:653px; max-width:100%; height:auto; display:block; margin:0 auto;" />

<img src="assets/images/image_lab4_shared_library_folders.png" alt="Figure" style="width:250px; max-width:100%; height:auto; display:block; margin:0 auto;" />

4. [INFO] Now, open the **Device Configurator** tool by clicking the **Configure Device** button in the **ModusToolbox™ for VS Code** extension

5. [INFO] The Device Configurator includes the following tabs:

- **PSOC™ Edge (PSE846GPS2DBZC4A):** configuration for PSOC™ Edge MCU, including:
    - **Peripherals**: configures peripherals including communication, digital, and system.
    - **Analog**: configures analog modules, including the autonomous analog, which includes ADC, PTComps, autonomous controller, CTBs, and PRB; and the low-power comparators.
    - **Pins**: configures the GPIOs, including Smart IO functionality.
    - **Clocks**: configures the device clock tree, including all clock inputs and outputs.
    - **System**: configures security, protection, and system settings such as debug and power modes.
    - **Memory**: configures the memory allocation and provides a graphical view of the memory map.
    - **DMA**: configures the device’s DMAs
- **AIROC™ Wi-Fi and Bluetooth® combo (CYW55513IUBG)**: configuration for CYW55513, including:
    - **Smart Coex library**: improves performance when using Bluetooth® and Wi-Fi at the same time
    - **Bluetooth® and Wi-Fi power configurators**: configures LPA settings

Feel free to navigate through the different tabs. The Device Configurator generates code only when the `design.modus` file is saved.

6. [INFO] In the Device Configurator, go to the **Pins** tab for the PSOC™ Edge MCU:

<img src="assets/images/image_lab4_device_configurator_pins.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

7. [INFO] In the PSOC™ Edge E84 EVK, the Wi-Fi Host wake-up signal is connected to **P11.4**, per the kit schematic:

<img src="assets/images/image_lab4_wifi_host_wake_schematic.png" alt="Figure" style="width:740px; max-width:100%; height:auto; display:block; margin:0 auto;" />

8. [INFO] Look for P11[4] in the Device Configurator and observe that it is named `CYBSP_WIFI_HOST_WAKE` and it is configured as follows:

    - Drive Mode: Open Drain, Drives Low. Input buffer on
    - Initial Drive State: High (1)
    - Interrupt Trigger Type: Rising Edge

> **Note:** P11.4 is already configured for Wi-Fi host wake-up in the WLAN Offloads example, but other projects can enable this functionality by configuring this pin accordingly.

<img src="assets/images/image_lab4_wifi_host_wake_pin_config.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

9. [INFO] Go to the CYW55513IUBG configuration panel

<img src="assets/images/image_lab4_wireless_device_config.png" alt="Figure" style="width:390px; max-width:100%; height:auto; display:block; margin:0 auto;" />

10. [INFO] Select the **Power→Wi-Fi** panel under **Power**.  
   Observe that the Host Wake functionality is enabled and uses `CYBSP_WIFI_HOST_WAKE`

> **Note:** Host Wake functionality is already enabled in the WLAN Offloads example, but other projects can enable this functionality by configuring this pin accordingly.

<img src="assets/images/image_lab4_lpa_host_wake_settings.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

11. [INFO] You can also observe that the rest of the configuration is disabled, including packet filters, which explains why the CPU is waking up on TCP ping packets

12. [INFO] Device Configurator includes direct links to documentation.
    Try clicking the link to the Low Power Assistant Documentation and feel free to navigate through the documentation

> **Note:** Some documentation links are local, while others require Internet access.

<img src="assets/images/image_lab4_lpa_documentation.png" alt="Figure" style="width:747px; max-width:100%; height:auto; display:block; margin:0 auto;" />

The WLAN Offloads example already includes LPA support; however, this functionality can be easily added to other projects by leveraging the **Device Configurator** and the **Library Manager**.

### Enable LPA offload functionality

This section demonstrates three LPA offload features: ARP offload, ICMP offload, and packet filter offload.

#### ARP offload

**Address Resolution Protocol (ARP)** is used to map an IP address (e.g., 192.168.1.1) to a physical machine address (e.g., ac:32:df:14:16:07). ARP uses broadcast frames to accomplish this.

ARP broadcast traffic is normally forwarded from the network to the device (Wi-Fi radio) and then to the host (application CPU) network stack. If the host is sleeping, the device wakes it up.

**ARP offload** is designed to improve the power consumption of the connected system by reducing the time the host needs to stay awake due to ARP broadcast traffic. Having the device handle some of the ARP traffic reduces how often the host wakes up from sleep, reducing host power consumption by allowing it to stay in the CPU Sleep and System Deep Sleep states longer.

**Peer Auto Reply** is a power-saving feature that allows the Wi-Fi device to reply to ARP requests from network peers without waking the host. Once the system connects to the network, the host instructs the Wi-Fi device to reply to ARP requests on its behalf while the host is in a low-power state, as shown in the following figure.

<img src="assets/images/image_lab4_arp_offload_flow.png" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

**ARP offload with auto reply** is enabled by default in the WLAN firmware for the PSOC™ Edge E84, so no additional configuration is required in the ModusToolbox™ Device Configurator.

Test this feature using the following steps:

1. Open a command prompt and use `nping` to send an ARP ping to the device’s IP address

```bash
nping --arp <IP address>
```

2. Observe that the CYW55513 responds to the request; however, it does not wake up the CPU.
   You should not observe activity in the serial terminal due to this request

<img src="assets/images/image_lab4_arp_ping_command.gif" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. Optionally, open Wireshark to observe the ARP packets

<img src="assets/images/image_lab4_arp_wireshark.gif" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

These steps demonstrated that the CYW55513 responds to ARP requests without waking up the CPU from Deep Sleep, which reduces power consumption and maximizes CPU bandwidth.

#### ICMP offload

The **Internet Control Message Protocol (ICMP)** is a network layer protocol used by network devices to diagnose network communication issues. ICMP is mainly used to determine whether or not data is reaching its intended destination by sending an ICMP request to the specific IP address and receiving the ICMP response in a timely manner.

With **ICMP offload**, WLAN firmware responds to the ICMP ECHO request by sending a PING ECHO response packet to the peer without waking up the host processor, allowing the host to stay in a sleep state for a longer duration.

<img src="assets/images/image_lab4_icmp_offload_flow.png" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

**ICMP offload** is enabled by default in the WLAN firmware for the PSOC™ Edge E84, so no additional configuration is required in the ModusToolbox™ Device Configurator.

Test this feature using the following steps:

1. Open a command prompt and use `nping` to send an ICMP ping to the device’s IP address

> **Note:** The `nping` command uses ICMP when no protocol is specified. Alternatively, the standard Windows `ping` command also uses ICMP.

```bash
nping --icmp <IP address>
```

2. Observe that the CYW55513 responds to the request; however, it does not wake up the CPU.
   There is no activity in the serial terminal due to this request

<img src="assets/images/image_lab4_icmp_ping_command.gif" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. Optionally, open Wireshark to observe the ICMP packets

<img src="assets/images/image_lab4_icmp_wireshark.gif" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

This section demonstrated that the CYW55513 responds to ICMP packets without waking up the CPU from Deep Sleep, which reduces power consumption and maximizes CPU bandwidth.

#### Packet filter offload

Whenever a WLAN packet is destined for the host, the WLAN processor must awaken the host (if it is asleep) so it can retrieve the packet for processing. Often, the host network stack processes the packet only to discover that it should be discarded because it is not needed. For example, it is destined for a port or service that is not being used. **Packet filters** allow these types of packets to be filtered and discarded by the WLAN processor so the host is not interrupted.

All packet filters are enabled after the Wi-Fi connection is established, so the application does not need to create any filters specifically to establish a connection.

The following types of packet filters are supported, based on a standard IP stack:

<img src="assets/images/image_lab4_packet_filter_types.png" alt="Figure" style="width:666px; max-width:100%; height:auto; display:block; margin:0 auto;" />

Earlier, you observed that the CPU wakes up on a TCP packet because packet filters are disabled by default. Enable them to observe the difference.

1. Open the **Device Configurator** and enable the packet filter functionality as shown below

    * **Minimal Set of Keep Filters** maintains a Wi-Fi connection with the access point, including:
        * ARP (0x806)
        * 802.1X (0x888E)
        * DHCP (68)
        * DNS (53)
    * **Enable Filter Configuration 0** is set by default to enable ARP packets (0x806)
    * **Enable Filter Configuration 1** is enabled to keep TCP packets sent and received using port 50007

> **Note:** Port 50007 must not be blocked by the firewall on your PC. Otherwise, select a different port.

<img src="assets/images/image_lab4_packet_filter_config.png" alt="Figure" style="width:432px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. Save the `design.modus` file (**Ctrl+S**) in the **Device Configurator**. This regenerates the configuration files. Close the **Device Configurator** after saving completes
3. Rebuild and reprogram the device
4. In the command prompt, use `nping` to send a TCP ping to the device’s IP address using port 50007
```bash
nping --tcp -g 50007 -p 50007 <IP address>
```

5. Observe that the device wakes up and responds to the request because packets to port 50007 are kept.
   You should observe activity in the serial terminal because the CPU wakes up

<img src="assets/images/image_lab4_filtered_tcp_ping_output.gif" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

6. Try sending a packet to a different TCP port and observe that the device does not wake up or respond, because the CYW55513 filters the packet

<img src="assets/images/image_lab4_blocked_tcp_port.gif" alt="Figure" style="width:800px; max-width:100%; height:auto; display:block; margin:0 auto;" />

7. Optionally, open Wireshark to observe the TCP packets

<img src="assets/images/image_lab4_filtered_tcp_wireshark.gif" alt="Figure" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

This demonstrates the packet filtering functionality, which is useful for keeping the host processor in a low-power mode for as long as possible to maximize CPU bandwidth and optimize power consumption.

### Other functionality supported by LPA

In addition to ARP offload, ICMP offload, and packet filter offload, the Low Power Assistant (LPA) middleware supports a broad set of networking and Bluetooth® low-power features that help reduce host MCU wakeups and extend system battery life. These features are configured through LPA and executed by the Wi-Fi & Bluetooth® combo device, allowing the PSOC™ Edge MCU to remain in low-power modes for longer periods.

Key capabilities include:

- **TCP keepalive offload**

    The TCP keepalive mechanism can be handled by the combo device without involving the host stack. This allows long-lived TCP connections (for example, to cloud services) to be maintained while the MCU stays in deep sleep, reducing both power and host processing overhead.

- **DHCP lease time renew offload**

    DHCP lease renewals can be performed by the wireless device itself. This prevents periodic host wakeups just to refresh the IP address lease, which is particularly useful for always-connected IoT nodes.

- **Neighbor Discovery (ND) offload**

    For IPv6 networks, Neighbor Discovery messages can be processed on the radio device, avoiding host intervention for routine link-layer address resolution and reachability checks.

- **NULL / NAT keepalive offload**

    The combo device can generate NULL and NAT keepalive traffic to keep Wi-Fi associations and NAT bindings alive through intermediate network equipment. This ensures connectivity is preserved even while the MCU is sleeping.

- **Wake on Wireless LAN (WoWLAN)**

    The radio can monitor incoming Wi-Fi traffic and wake the MCU on specific events (such as a magic packet, pattern match, or other configured triggers), enabling ultra-low-power idle states while still remaining reachable over the network.

- **MQTT keepalive offload**

    For MQTT-based applications, keepalive messages can be managed by the wireless device, allowing persistent broker connections with minimal MCU activity—ideal for sensor nodes that send data intermittently.

- **Bluetooth® low power**

    On the Bluetooth® side, LPA supports low-power modes and offloads that reduce host wakeups during advertising, scanning, and connection maintenance. This is especially important for battery-powered peripherals and audio devices where both Wi-Fi and Bluetooth® may be active.

### Conclusion

You successfully executed the WLAN Offloads example to enable and evaluate some of the capabilities of the Low-Power Assistant library. This functionality can be easily integrated into any PSOC™ Edge application that uses the CYW55513, and demonstrates how LPA provides a comprehensive framework for managing low-power behavior across Wi-Fi, TCP/IP, MQTT, and Bluetooth® stacks. By shifting routine protocol maintenance and link-keepalive tasks from the PSOC™ Edge MCU to the combo device, LPA helps designers meet aggressive power budgets while maintaining robust, always-on connectivity.

For battery-powered and energy-harvested products, every unnecessary host wakeup shortens battery life or forces the use of a larger, more expensive battery. By letting the CYW55513 filter and answer routine network traffic on its own, LPA can meaningfully extend the time between charges or battery replacements for connected sensors, wearables, and other always-on IoT devices, without sacrificing responsiveness to real application traffic and without requiring custom low-power firmware to be developed and validated from scratch.

## Appendix A: Creating a PSOC™ Edge application in ModusToolbox™

<span id="appendix-a-creating-a-psoc-edge-application-in-modustoolbox"></span>

The following steps show how to create a new project for PSOC™ Edge in ModusToolbox™, using either Visual Studio Code (VS Code) or the command-line interface (CLI).

### Creating an application using VS Code

This appendix shows a quick workflow to create and open a new PSOC™ Edge application using ModusToolbox™ and Visual Studio Code.

1. Open **ModusToolbox™ Dashboard**. Select **Microsoft Visual Studio Code** as the target IDE, and then launch **Project Creator**.
    If you have not installed ModusToolbox™ or Visual Studio Code, see the [Required development tools and prerequisites](#required-development-tools-and-prerequisites) section

<img src="assets/images/MTB_Dashboard.png" alt="ModusToolbox™ Dashboard" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

2. Choose the PSOC™ Edge BSP that matches your kit. This training uses the **KIT_PSE84_EVAL_EPC2** (selected below) or the **KIT_PSE84_AI**. Click **Next** after selecting the BSP

<img src="assets/images/image_appendix_a_bsp_selection.png" alt="Project Creator start screen" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

3. Select a **path** for your project, make sure the **target IDE** is Microsoft Visual Studio Code, and then select the application for the lab you are following (see each lab's **Project creation** section for the specific application and category).
   Optionally, rename the project as desired and click **Create**

> **Warning:** Windows has a 260-character path length limit. A long workspace path and/or a long project name may cause build issues when creating some applications.

<img src="assets/images/image_appendix_a_application_selection.png" alt="Project Creator target and template selection" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

4. After project generation completes, open the generated workspace in Visual Studio Code by opening the `.code-workspace` file created in the project folder

<img src="assets/images/VSC_open1.png" alt="Open generated project in VS Code" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

> **Note:** Open the workspace in trust mode to launch the **ModusToolbox™ for VS Code** extension.

<img src="assets/images/VSC_open3.png" alt="open workspace in trust mode" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

5. The project is imported, and you should see all the files in Visual Studio Code. Use the **ModusToolbox™ for VS Code** extension to build and program the project

> **Note:** The **ModusToolbox™ for VS Code** extension might show some warnings. Click the corresponding buttons to fix the configuration issues.

<img src="assets/images/VSC_open2.png" alt="Verify project in VS Code" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

#### Using LLVM compiler in Visual Studio Code

1. Download LLVM from the [LLVM Embedded Toolchain for Arm releases page](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/)
2. Unzip to the user directory. For example:
   Windows: `C:\Users\<user>\LLVM-ET-Arm-19.1.5-Windows-x86_64`
3. Go to the **Settings** tab in the **ModusToolbox™ for VS Code** extension, select **LLVM_ARM** as the **Toolchain**, and then select the corresponding **LLVM Compiler Path**

<img src="assets/images/MTB_VSC_LLVM.png" alt="LLVM toolchain configuration in the ModusToolbox™ for VS Code extension" style="width:760px; max-width:100%; height:auto; display:block; margin:0 auto;" />

### Creating an application using the CLI

ModusToolbox™ also includes a `project-creator-cli` tool so you can create applications without a GUI, or automate project creation in scripts.

> **Note:** On Windows, run CLI commands from the **modus-shell** (Cygwin) terminal included with the ModusToolbox™ installation, instead of a standard Windows command-line application. This shell provides access to all ModusToolbox™ tools. On Linux and macOS, you can use any terminal application.

1. Open a modus-shell (Windows) or terminal (Linux/macOS) window, and navigate to the folder where you want to create the application

2. Run the following command to list the available code examples for the **KIT_PSE84_EVAL_EPC2** BSP:

```bash
project-creator-cli --list-apps KIT_PSE84_EVAL_EPC2
```

3. Run the following command to create the application selected for the lab you are following, replacing `<app-id>` and `<project-name>` accordingly:

```bash
project-creator-cli --board-id KIT_PSE84_EVAL_EPC2 --app-id <app-id> --user-app-name <project-name>
```

4. You can build, program, and open other ModusToolbox™ tools directly from the modus-shell or terminal, for example, `make build`, `make program`, `make device-configurator`, or `make library-manager`.
    Visit the [ModusToolbox™ user guide](https://www.infineon.com/modustoolboxuserguide) for more information

#### Using LLVM compiler in the CLI

1. Download LLVM from the [LLVM Embedded Toolchain for Arm releases page](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/)
2. Unzip to the user directory. For example:
   Windows: `C:\Users\<user>\LLVM-ET-Arm-19.1.5-Windows-x86_64`
3. Define the environment variable `CY_COMPILER_LLVM_ARM_DIR` to point to this directory, based on your operating system

## Appendix B: KIT_PSE84_EVAL details

<img src="assets/images/image_appendix_b_evk_board_overview.png" alt="KIT_PSE84_EVAL board overview" style="width:757px; max-width:100%; height:auto; display:block; margin:0 auto;" />

| No. | Item | No. | Item |
| ---: | --- | ---: | --- |
| 1 | Baseboard power LED (D1) | 26 | 3-axis magnetometer (U4) |
| 2 | KitProg3 program/debug USB-C connector (J8) | 27 | Raspberry Pi-compatible display capacitive touch connector (J41)** |
| 3 | PSOC™ 5LP-based KitProg3 programmer and debugger (CY8C5868LTI-LP039, U2) | 28 | Linear potentiometer (R34) |
| 4 | Reset button (SW1) | 29 | Analog microphones (IM73A135V01XTSA1, U36 and U37)** |
| 5 | KitProg3 status LED (D2) | 30 | User LEDs (D3, D4, D5) |
| 6 | PSOC™ Edge E84 MCU ETM/JTAG debug and trace header (J15) | 31 | Thermistor (TH1) |
| 7 | PSOC™ Edge E84 MCU 10-pin SWD/JTAG program and debug header (J16) | 32 | CAPSENSE™ buttons and slider (CSB1, CSB2, CSS1) |
| 8 | Alternative serial interface configuration headers (J20, J21) | 33 | BOOT configuration switch (SW6) |
| 9 | PSOC™ Edge E84 MCU USB host Type-A connector (J27) | 34 | Proximity sense connector (J19) |
| 10 | USB-C power delivery (PD) fault LED (D6) | 35 | I/O headers compatible with Arduino UNO R3 (J2, J3, J4) |
| 11 | Custom display capacitive touch panel connector (J37)** | 36 | Alternative serial interface I/O header (J14)* |
| 12 | PSOC™ Edge E84 MCU USB-C connector (J30) | 37 | Power header compatible with Arduino UNO R3 (J1) |
| 13 | External power supply VIN connector (J31) | 38 | PSOC™ Edge E84 MCU expansion I/O headers (J6, J7, J40)* |
| 14 | PSOC™ Edge E84 MCU user buttons (SW2, SW4) | 39 | MicroSD card holder (J35)** |
| 15 | M.2 (B-key) memory interface connector (J29) | 40 | Infineon's Shield2Go interface headers (J10, J12)* |
| 16 | 128 Mbit Octal-SPI HYPERRAM™ (S70KS1283GABHI020, U12)*** | 41 | Analog microphones (IM73A135V01XTSA1, U36 and U37)** |
| 17 | Processor System on module (SoM) 260-pin SODIMM connector (J28) | 42 | mikroBUS compatible headers by Mikroelektronika (J9, J17)* |
| 18 | CYW55513 tri-band (Wi-Fi & Bluetooth®) combo radio (U3) section | 43 | Extended I2S header (J11)* |
| 19 | Processor System on module (SoM) power LED (D3) | 44 | 6-axis accelerometer and gyroscope IMU (U5) |
| 20 | 1-Gb Octal-SPI NOR flash (S28HS01GTGZBH1030, U10)*** | 45 | M.2 (E-key) radio interface connector (J13) |
| 21 | 128-Mb Quad-SPI NOR flash (S25FS128SAGMFB100, U11) | 46 | PSOC™ Edge E84 MCU power selection/monitoring headers (J18, J22, J23, J24, J25, J26) |
| 22 | MIPI-DSI custom display connector (J38)** | 47 | Headphone connector (J34)* |
| 23 | PSOC™ Edge E84 MCU (PSE846GPS2DBZC4A, U1) | 48 | Speaker (ACC6) |
| 24 | PSOC™ 4000T CAPSENSE™ Co-processor (U9)*** | 49 | RJ45 Ethernet MagJack connector (J5)* |
| 25 | Raspberry Pi-compatible MIPI-DSI display connector (J39)** | 50 | KitProg3 programming mode selection button (SW3) |

\*Footprint only, not populated on the board

\*\*Component at the bottom side of the Baseboard

\*\*\*Component at the bottom side of the SoM

## Appendix C: KIT_PSE84_AI details

The KIT_PSE84_AI shares the same PSOC™ Edge E84 MCU and CYW55513 Wi-Fi & Bluetooth® combo device used in this training, in a board form factor optimized for machine learning, audio, and sensor fusion applications.

<div style="display:flex; gap:16px; justify-content:center; align-items:flex-start;">

<img src="assets/images/image_appendix_c_ai_board_top.svg" alt="KIT_PSE84_AI board - top view" style="width:48%; max-width:380px; height:auto;" />

<img src="assets/images/image_appendix_c_ai_board_bottom.svg" alt="KIT_PSE84_AI board - bottom view" style="width:48%; max-width:380px; height:auto;" />

</div>

| No. | Item | No. | Item |
| ---: | --- | ---: | --- |
| 1 | PSOC™ Edge E84 MCU USB Device/Host connector (J2) | 17 | PSOC™ Edge E84 MCU 10-pin SWD/JTAG program and debug header (J4) |
| 2 | PSOC™ Edge E84 MCU user button (SW1) | 18 | 128-Mbit Octal-SPI HYPERRAM™ (S70KS1283GABHI020, U3) |
| 3 | 6-axis accelerometer and gyroscope (U18) | 19 | DVP camera interface header (J14) |
| 4 | KitProg3 program/debug USB-C connector (J1) | 20 | Battery input connector (J3) |
| 5 | KitProg3 status LED (LED4) | 21 | Power LED (LED5) |
| 6 | User LEDs (LED1, LED2, LED3) | 22 | XENSIV™ digital MEMS microphones (IM73D122V01, U7, U8) |
| 7 | Digital humidity and temperature sensor (U22) | 23 | Speaker connector output (J8) |
| 8 | XENSIV™ 60 GHz RADAR sensor (BGT60TR13C, U5) | 24 | Audio DAC and amplifier (U28) |
| 9 | XENSIV™ digital barometric pressure sensor (DPS368, U6) | 25 | 512-Mbit Quad-SPI NOR flash (S25HS512TFABHI013, U4) |
| 10 | PSOC™ Edge E84 MCU reset button (SW2) | 26 | Analog microphones (IM73A135V01XTSA1, U9, U10) |
| 11 | 3-axis magnetometer (U20) | 27 | Display power and capacitive touch input connector (J10) |
| 12 | PSOC™ Edge E84 MCU (PSE846GPS2DBZC4A, U1) | 28 | I2C (QWIIC) interface connector (J7) |
| 13 | Expansion I/O header (J5) | 29 | Analog I/O interface header (J17) |
| 14 | CYW55513-based Murata Type 2FY module (U25) | 30 | KitProg3 (PSOC™ 5LP) programmer and debugger (CY8C5868LTI-LP039, U2) |
| 15 | Wi-Fi/Bluetooth® antenna (ANT1) | 31 | KitProg3 programming mode selection button (SW3) |
| 16 | Raspberry Pi-compatible MIPI-DSI display connector (J10) | | |

## Revision history

<table class="no-center-table">
  <thead>
    <tr>
      <th>Document revision</th>
      <th>Date</th>
      <th>Description of changes</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>**</td>
      <td>2026-10-04</td>
      <td>Initial release</td>
    </tr>
  </tbody>
</table>

### Disclaimer

All referenced product or service names and trademarks are the property of their respective owners.

The Bluetooth&reg; word mark and logos are registered trademarks owned by Bluetooth SIG, Inc., and any use of such marks by Infineon is under license.

PSOC&trade;, formerly known as PSoC&trade;, is a trademark of Infineon Technologies. Any references to PSoC&trade; in this document or others shall be deemed to refer to PSOC&trade;.

---------------------------------------------------------

© Cypress Semiconductor Corporation, 2023-2026. This document is the property of Cypress Semiconductor Corporation, an Infineon Technologies company, and its affiliates ("Cypress"). This document, including any software or firmware included or referenced in this document ("Software"), is owned by Cypress under the intellectual property laws and treaties of the United States and other countries worldwide. Cypress reserves all rights under such laws and treaties and does not, except as specifically stated in this paragraph, grant any license under its patents, copyrights, trademarks, or other intellectual property rights. If the Software is not accompanied by a license agreement and you do not otherwise have a written agreement with Cypress governing the use of the Software, then Cypress hereby grants you a personal, non-exclusive, nontransferable license (without the right to sublicense) (1) under its copyright rights in the Software (a) for Software provided in source code form, to modify and reproduce the Software solely for use with Cypress hardware products, only internally within your organization, and (b) to distribute the Software in binary code form externally to end users (either directly or indirectly through resellers and distributors), solely for use on Cypress hardware product units, and (2) under those claims of Cypress's patents that are infringed by the Software (as provided by Cypress, unmodified) to make, use, distribute, and import the Software solely for use with Cypress hardware products. Any other use, reproduction, modification, translation, or compilation of the Software is prohibited.
<br>
TO THE EXTENT PERMITTED BY APPLICABLE LAW, CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO THIS DOCUMENT OR ANY SOFTWARE OR ACCOMPANYING HARDWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. No computing device can be absolutely secure. Therefore, despite security measures implemented in Cypress hardware or software products, Cypress shall have no liability arising out of any security breach, such as unauthorized access to or use of a Cypress product. CYPRESS DOES NOT REPRESENT, WARRANT, OR GUARANTEE THAT CYPRESS PRODUCTS, OR SYSTEMS CREATED USING CYPRESS PRODUCTS, WILL BE FREE FROM CORRUPTION, ATTACK, VIRUSES, INTERFERENCE, HACKING, DATA LOSS OR THEFT, OR OTHER SECURITY INTRUSION (collectively, "Security Breach"). Cypress disclaims any liability relating to any Security Breach, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any Security Breach. In addition, the products described in these materials may contain design defects or errors known as errata which may cause the product to deviate from published specifications. To the extent permitted by applicable law, Cypress reserves the right to make changes to this document without further notice. Cypress does not assume any liability arising out of the application or use of any product or circuit described in this document. Any information provided in this document, including any sample design information or programming code, is provided only for reference purposes. It is the responsibility of the user of this document to properly design, program, and test the functionality and safety of any application made of this information and any resulting product. "High-Risk Device" means any device or system whose failure could cause personal injury, death, or property damage. Examples of High-Risk Devices are weapons, nuclear installations, surgical implants, and other medical devices. "Critical Component" means any component of a High-Risk Device whose failure to perform can be reasonably expected to cause, directly or indirectly, the failure of the High-Risk Device, or to affect its safety or effectiveness. Cypress is not liable, in whole or in part, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any use of a Cypress product as a Critical Component in a High-Risk Device. You shall indemnify and hold Cypress, including its affiliates, and its directors, officers, employees, agents, distributors, and assigns harmless from and against all claims, costs, damages, and expenses, arising out of any claim, including claims for product liability, personal injury or death, or property damage arising from any use of a Cypress product as a Critical Component in a High-Risk Device. Cypress products are not intended or authorized for use as a Critical Component in any High-Risk Device except to the limited extent that (i) Cypress's published data sheet for the product explicitly states Cypress has qualified the product for use in a specific High-Risk Device, or (ii) Cypress has given you advance written authorization to use the product as a Critical Component in the specific High-Risk Device and you have signed a separate indemnification agreement.
<br>
Cypress, the Cypress logo, and combinations thereof, ModusToolbox, PSoC, CAPSENSE, EZ-USB, F-RAM, and TRAVEO are trademarks or registered trademarks of Cypress or a subsidiary of Cypress in the United States or in other countries. For a more complete list of Cypress trademarks, visit www.infineon.com. Other names and brands may be claimed as property of their respective owners.
[Click here](../README.md) to view the README.

## Design and implementation

All PSOC&trade; Edge E84 MCU applications have a dual-CPU three-project structure to develop code for the CM33 and CM55 cores. The CM33 core has two separate projects for the secure processing environment (SPE) and non-secure processing environment (NSPE). A project folder consists of various subfolders, each denoting a specific aspect of the project. The three project folders are as follows:

**Table 1. Application projects**

Project      | Description
-------------|------------------------
*proj_cm33_s* | Project for CM33 secure processing environment (SPE)
*proj_cm33_ns* | Project for CM33 non-secure processing environment (NSPE)
*proj_cm55* | CM55 project

<br>

In this code example, at device reset, the secure boot process starts from the ROM boot with the secure enclave (SE) as the root of trust (RoT). From the secure enclave, the boot flow is passed on to the system CPU subsystem where the secure CM33 application starts. After all necessary secure configurations, the flow is passed on to the non-secure CM33 application. Resource initialization for this example is performed by this CM33 non-secure project. It configures the system clocks, pins, clock to peripheral connections, and other platform resources. It then enables the CM55 core using the `Cy_SysEnableCM55()` function.

In the CM33 non-secure application, the clocks and system resources are initialized by the BSP initialization function, and then, the CM55 application is launched. The CM55 application is responsible for the complete operation of the Broadcast Sink functionality.

The CM55 application initializes the system resources. The retarget-io middleware is configured to use the debug UART and the user LED is initialized. Then the Bluetooth&reg; host stack is initialized along with the creation of RTOS tasks for I2S audio playback and LC3 decoding.

The sink scans for broadcast announcements automatically without a fixed broadcast-ID filter.
It synchronizes to periodic advertising, parses the Basic Audio Announcement (BASE) and
BIGInfo, and establishes Broadcast Isochronous Group (BIG) synchronization. It then opens
the controller-to-host HCI data path for Broadcast Isochronous Stream (BIS) reception.

Received LC3 frames are copied into static circular buffers and queued to the LC3 decoder
task. With `ENABLE_LC3_DECODING=0` (default), the application receives data without sound.
Set `ENABLE_LC3_DECODING=1` in common.mk and add the Audio SW Codecs Tech
Pack library to the CM55 project to decode received frames into PCM audio.

Post decoding, the PCM audio data, for each channel, is passed through jitter management algorithm based on Asynchronous Sample Rate Converter (ASRC) to ensure that the audio is played out at the correct rate. The processed PCM audio data is then transmitted to the TLV320DAC3100 hardware codec over the I2S interface for playback over the onboard speaker.

`AUDIO_CONFIG=MONO` decodes the first channel; `AUDIO_CONFIG=STEREO` decodes two channel
frames carried in one BIS. The companion source provides dual-mono stereo content and
the onboard speaker plays only the left channel. A second separate BIS is raw receive only.


### Memory Map Customization

Static LC3 receive buffers, decoder state, PCM buffers, and RTOS resources consume internal
memory according to the selected audio configuration. Preserve the supplied memory and
linker configuration when changing decoder or channel settings; clean and rebuild the
complete application and check the resulting linker map for memory usage.

<br>

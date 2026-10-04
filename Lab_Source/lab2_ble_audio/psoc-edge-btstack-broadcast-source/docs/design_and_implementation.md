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

In the CM33 non-secure application, the clocks and system resources are initialized by the BSP initialization function, and then, the CM55 application is launched. The CM55 application is responsible for the complete operation of the Broadcast Source functionality.

The CM55 application initializes the system resources. The retarget-io middleware is configured to use the debug UART and the user LED is initialized.
Then the Bluetooth&reg; host stack is initialized under FreeRTOS. Extended advertising announces *Broadcast Source* and Broadcast ID `0x10`.
Periodic advertising carries the Basic Audio Announcement (BASE), describing the codec and channel allocation.
The application creates one Broadcast Isochronous Group (BIG) containing one Broadcast Isochronous Stream (BIS).

After BIG creation, the source configures the HCI ISO input data path to the controller over HCI UART.
Data-path setup completion starts a four-SDU priming burst; controller packet-completion credits drive subsequent transmission.
The success message confirms controller-completed transmission, not synchronization or playback at a sink.

With `ENABLE_LC3_ENCODING=0` (default), 651 pre-encoded LC3 frames repeat every 6.51 seconds at 48 kHz, 10 ms, and 100 bytes per channel.
With `ENABLE_LC3_ENCODING=1`, Google liblc3 v1.0.3 encodes the built-in 48 kHz PCM recording at runtime.
Neither mode uses external audio input; runtime encoding does not resample the PCM recording.

- `ENABLE_LC3_ENCODING=0` excludes the PCM array and runtime liblc3 implementation from the build.
- `ENABLE_LC3_ENCODING=1` includes PCM and encoder storage without changing the existing stack, pool, or alignment settings.

Set `AUDIO_CONFIG=MONO` (default) or `AUDIO_CONFIG=STEREO` in common.mk, matching the sink.
Both modes use one BIS: MONO carries one 100-byte LC3 frame, while STEREO carries the left frame followed by the right frame in one 200-byte SDU.
The bundled sample supplies the same content to both channels in STEREO mode (dual mono).

<br>

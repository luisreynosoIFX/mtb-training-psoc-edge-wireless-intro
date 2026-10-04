/******************************************************************************
* File Name        : jitter_mgmt.c
*
* Description      : This file contains the source code for audio jitter
*                    management for smooth audio playback over I2S.
*
* Related Document : See README.md
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "jitter_mgmt.h"
#include "audio_driver_psoc.h"
#include "audio_driver_i2s_task.h"
#include <stdint.h>

#if (ENABLE_JITTER_MGMT_ASRC == 1)
    #include "IFX_asrc.h"
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

/******************************************************************************
* Macros
*******************************************************************************/

#define CHECK_RESULT(result)     do                                          \
                                 {                                           \
                                     CY_ASSERT(CY_RSLT_SUCCESS == result);   \
                                 }                                           \
                                 while(0)


/******************************************************************************
* Global Variables
*******************************************************************************/

#if (ENABLE_JITTER_MGMT_ASRC == 1)
static IFX_ASRC_STRUCT_t jitter_mgmt_asrc_mem[JITTER_MGMT_ASRC_MAX_CHANNELS];

static int32_t asrc_in_i2s_tx_buffer[(JITTER_MGMT_ASRC_MAX_FRAME_BUFFER_SIZE / sizeof(int16_t) + 10u)] = {0};
static int32_t asrc_out_i2s_tx_buffer[JITTER_MGMT_ASRC_OUTPUT_BUFFER_SIZE] = {0};

static int32_t asrc_cdps_value = 0;

static uint32_t jitter_mgmt_num_channels = 1;
static uint32_t jitter_mgmt_frame_size_samples = 0;
static int32_t  jitter_mgmt_buffer_num_samples = 0;

static int32_t  watermark_level_average = 0;
static int32_t  watermark_level = 0;
static int32_t  watermark_monitor_count = 0;
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */


/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: jitter_mgmt_init
*******************************************************************************
* Summary:
*   Function that initializes the ASRC module for the jitter management along
*   with setting up of required variables and flags.
*
* Parameters:
*   num_channels      : Number of channels for jitter management
*   sampling_rate_hz  : Sampling rate in Hz of the PCM audio
*   frame_duration_us : Audio frame duration in microseconds (unused)
*   frame_size_bytes  : Decoded PCM frame size in bytes per channel
*
* Return:
*   None
*
*******************************************************************************/
void jitter_mgmt_init(uint32_t num_channels, uint32_t sampling_rate_hz,
                      uint32_t frame_duration_us, uint32_t frame_size_bytes)
{
#if (ENABLE_JITTER_MGMT_ASRC == 1)
    if (num_channels > JITTER_MGMT_ASRC_MAX_CHANNELS)
    {
        printf(">> Error: Failed to init jitter management! Exceeded max "
                "channels: '%lu' > '%lu'\n", (unsigned long) num_channels,
                (unsigned long) JITTER_MGMT_ASRC_MAX_CHANNELS);
        return;
    }

    jitter_mgmt_num_channels = num_channels;
    jitter_mgmt_buffer_num_samples = 0;
    watermark_level = 0;
    watermark_level_average = 0;
    jitter_mgmt_frame_size_samples = frame_size_bytes / sizeof(int16_t);

    for (uint32_t i = 0; i < num_channels; i++)
    {
        init_IFX_asrc(&(jitter_mgmt_asrc_mem[i]), sampling_rate_hz,
                        sampling_rate_hz);

        asrc_cdps_value = 0 * (1 << Q_CDPS);
        IFX_SetClockDrift(&(jitter_mgmt_asrc_mem[i]), asrc_cdps_value);
    }
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */
}

/******************************************************************************
* Function Name: jitter_mgmt_process_data
*******************************************************************************
* Summary:
*   Function that processes the given PCM audio data using the ASRC module for
*   jitter management. This is done for all channels one at a time.
*
* Parameters:
*   jitter_mgmt_data      : Structure containing input and output data pointers
*                           for multiple channels
*   num_channels          : Number of channels for jitter management processing
*   data_size_per_channel : Size of input audio data in bytes for each channel
*
* Return:
*   uint32_t - Actual output size in bytes per channel post jitter processing
*
*******************************************************************************/
uint32_t jitter_mgmt_process_data(jitter_mgmt_data_t *jitter_mgmt_data,
                                  uint32_t num_channels,
                                  uint32_t data_size_per_channel)
{
    uint32_t output_size_per_channel = 0;

#if (ENABLE_JITTER_MGMT_ASRC == 1)
    int16_t* frame_in_buffer;
    int16_t* frame_out_buffer;
    uint32_t asrc_out_len = 0;
    uint32_t min_output_size = UINT32_MAX;
    uint32_t max_variation = 0;
    uint32_t variation = 0;
    uint32_t channel_output_sizes[JITTER_MGMT_ASRC_MAX_CHANNELS] = {0};

    if (num_channels > JITTER_MGMT_ASRC_MAX_CHANNELS)
    {
        printf(">> Error: Jitter management process failed! Exceeded max "
                "channels: '%lu' > '%lu'\n", (unsigned long) num_channels,
                (unsigned long) JITTER_MGMT_ASRC_MAX_CHANNELS);
        return 0;
    }

    /* Process all channels and store their output sizes */
    for (uint32_t ch = 0; ch < num_channels; ch++)
    {
        frame_in_buffer = (int16_t *) jitter_mgmt_data[ch].input_data_ptr;
        frame_out_buffer = (int16_t *) jitter_mgmt_data[ch].output_data_ptr;

        /* Convert input samples from int16_t to int32_t */
        for (uint32_t i = 0; i < (data_size_per_channel / sizeof(int16_t)); i++)
        {
            asrc_in_i2s_tx_buffer[i] = (int32_t) (*(int16_t *)frame_in_buffer);
            frame_in_buffer++;
        }

        asrc_out_len = OUTPUT_BUFFER_MAX;

        /* Process the channel through ASRC */
        IFX_asrc(asrc_in_i2s_tx_buffer,
                 (uint16_t)(data_size_per_channel / sizeof(int16_t)),
                 asrc_out_i2s_tx_buffer, (uint16_t *) &asrc_out_len,
                 &(jitter_mgmt_asrc_mem[ch]));

        /* Store the output size for this channel */
        channel_output_sizes[ch] = asrc_out_len * sizeof(int16_t);
        
        /* Keep track of the minimum output size across all channels */
        if (channel_output_sizes[ch] < min_output_size)
        {
            min_output_size = channel_output_sizes[ch];
        }

        /* Convert output samples from int32_t to int16_t */
        for (uint32_t i = 0; i < asrc_out_len; i++)
        {
            *(frame_out_buffer++) = (int16_t) asrc_out_i2s_tx_buffer[i];
        }
    }

    /* Use the minimum output size to ensure all channels have the same length */
    output_size_per_channel = min_output_size;

    /* Log size variations for debugging */
    if (num_channels > 1)
    {
        for (uint32_t ch = 0; ch < num_channels; ch++)
        {
            variation = (channel_output_sizes[ch] > min_output_size) ?
                        (channel_output_sizes[ch] - min_output_size) : 0;
            if (variation > max_variation)
            {
                max_variation = variation;
            }
        }

        /* Only log if variation is significant (more than 2 samples per channel) */
        if (max_variation > (JITTER_MGMT_ASRC_MAX_VARIATION_SAMPLES * sizeof(int16_t)))
        {
            printf(">> Info: ASRC output size variation: %lu bytes (using min: %lu)\n",
                   (unsigned long) max_variation, (unsigned long) min_output_size);
        }
    }

#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

    return output_size_per_channel;
}

/******************************************************************************
* Function Name: jitter_mgmt_update_buffer_level
*******************************************************************************
* Summary:
*   Function that updates the buffer level based on the input argument.
*
* Parameters:
*   num_samples : Number of PCM samples added (+ve) or consumed (-ve)
*
* Return:
*   None
*
*******************************************************************************/
void jitter_mgmt_update_buffer_level(int32_t num_samples)
{
#if (ENABLE_JITTER_MGMT_ASRC == 1)
    jitter_mgmt_buffer_num_samples += num_samples;
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */
}

/******************************************************************************
* Function Name: jitter_mgmt_update_asrc_state
*******************************************************************************
* Summary:
*   Function that updates the ASRC state based on current buffer levels.
*   The buffer levels are averaged over a period of frames and then the
*   ASRC state (CDPS) is updated periodically once averaging is done.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void jitter_mgmt_update_asrc_state(void)
{
#if (ENABLE_JITTER_MGMT_ASRC == 1)
    /* Dynamic CDPS Adjustment for ASRC */
    watermark_level = jitter_mgmt_buffer_num_samples / (int32_t) jitter_mgmt_frame_size_samples;
    watermark_level -= ((int32_t) I2S_TX_INITIAL_FRAME_DELAY - 2);
    watermark_level_average += watermark_level;

    watermark_monitor_count ++;

    if (JITTER_MGMT_WATERMARK_MONITOR_COUNT_US <= watermark_monitor_count)
    {
        watermark_level_average /= ((int32_t) watermark_monitor_count);
        watermark_monitor_count = 0;

        if (watermark_level_average == 0)
        {
            asrc_cdps_value = 0 * (1 << Q_CDPS);
        }
        else if (watermark_level_average > 0)
        {
            asrc_cdps_value = -60 * (1 << Q_CDPS);
            asrc_cdps_value &= 0xFFFFFFFE;
        }
        else
        {
            asrc_cdps_value = 60 * (1 << Q_CDPS);
            asrc_cdps_value &= 0xFFFFFFFE;
        }

        for (uint32_t i = 0; i < jitter_mgmt_num_channels; i++)
        {
            /* Note, in this application, the CDPS setting is common to all
             * channels.
             */
            IFX_SetClockDrift(&(jitter_mgmt_asrc_mem[i]), asrc_cdps_value);
        }
    }
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */
}

/* [] END OF FILE */

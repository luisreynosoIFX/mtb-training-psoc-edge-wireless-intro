/******************************************************************************
* File Name        : audio_decoder_task.c
*
* Description      : This file contains the source code for the RTOS task that
*                    performs LC3 decoding for the LE Audio Broadcast Sink example.
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
#include "audio_decoder_task.h"

#include "cy_pdl.h"
#include "cybsp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"

#include "lc3_codec.h"
#include "wiced_memory.h"
#include "audio_driver_psoc.h"
#include "audio_driver_i2s_task.h"
#include "jitter_mgmt.h"


/******************************************************************************
* Macros
*******************************************************************************/
/* Instance indices for left and right channels of LC3 decoder */
#define LC3_DECODER_LEFT_CHANNEL_INDEX      (0)
#define LC3_DECODER_RIGHT_CHANNEL_INDEX     (1)

/* Size of static buffer in bytes for LC3 data (pre LC3 decoding) */
#define LC3_ENCODED_DATA_BUFFER_SIZE        (1500)


/******************************************************************************
* Global Variables
*******************************************************************************/
/* Audio decoder task handle */
TaskHandle_t audio_decoder_task_handle;

/* Handle of the queue holding the Audio Decoder task commands. */
QueueHandle_t audio_decoder_task_q;


/* Static buffer for LC3 data (pre LC3 decoding) */
static uint8_t lc3_encoded_data_buffer[LC3_ENCODED_DATA_BUFFER_SIZE] = {0};
static uint32_t lc3_encoded_data_buffer_index = 0;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void audio_decoder_task(void* pvParameters);


/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: create_audio_decoder_task
*******************************************************************************
* Summary:
*   Function that creates the Audio Decoder RTOS task.
*
* Parameters:
*   None
*
* Return:
*   CY_RSLT_SUCCESS upon successful creation of the task, else a non-zero value
*   that indicates the error.
*
*******************************************************************************/
cy_rslt_t create_audio_decoder_task(void)
{
    BaseType_t status;

    status = xTaskCreate(audio_decoder_task, "Audio Dec Task",
                         AUDIO_DECODER_TASK_STACK_SIZE, NULL,
                         AUDIO_DECODER_TASK_PRIORITY, &audio_decoder_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t) status;
}

/******************************************************************************
* Function Name: audio_decoder_task
*******************************************************************************
* Summary:
*   Task that handles the LC3 decoder - initialization, de-initialization,
*   resetting, and decoding. The configuration and data are received over an
*   RTOS queue. The decoded data is then passed to the I2S audio driver.
*
* Parameters:
*   pvParameters : Task parameter defined during task creation (unused)
*
* Return:
*   None
*
*******************************************************************************/
static void audio_decoder_task(void* pvParameters)
{
    audio_decoder_q_data_t audio_decoder_q_data;
    lc3_decoder_info_t* decoder_info_ptr;
    bool decoder_initialized = false;
    uint32_t decoded_data_size = 0;
    uint8_t *lc3_data_r;
    uint8_t *lc3_data_l;

#if ENABLE_LC3_DECODING
    lc3_decoder_config_t* codec_config_ptr;
    lc3_config_t lc3_codec_config;
    bool right_channel_enabled = false;
#else
    uint32_t total_bytes = 0;
    bool reception_reported = false;
#endif /* ENABLE_LC3_DECODING */

    /* Remove warning for unused parameter */
    (void)pvParameters;

    printf("Audio Decoder Task started...\n");

    /* Create an RTOS queue to communicate data / commands between various tasks
     * and callbacks.
     */
    audio_decoder_task_q = xQueueCreate(AUDIO_DECODER_TASK_QUEUE_LENGTH,
                                        sizeof(audio_decoder_q_data_t));

    for (;;)
    {
        /* Receive the command and data from the queue and process it. */
        if (pdTRUE == xQueueReceive(audio_decoder_task_q, &audio_decoder_q_data,
                                    portMAX_DELAY))
        {
            switch(audio_decoder_q_data.cmd)
            {
                case LC3_DECODER_INIT:
                {
#if ENABLE_LC3_DECODING
                    if (decoder_initialized)
                    {
                        printf(">> [%s]: LC3_DECODER_INIT: LC3 Decoder already initialized!\n",
                               __FUNCTION__);
                        break;
                    }

                    /* Parse the LC3 Decoder configurations and initialize the decoder. */
                    codec_config_ptr = (lc3_decoder_config_t*)
                                       &audio_decoder_q_data.decoder.config;

                    lc3_codec_config.sampleRate = codec_config_ptr->sampleRate;
                    lc3_codec_config.sduInterval = codec_config_ptr->sduInterval;
                    lc3_codec_config.octetsPerFrame = codec_config_ptr->octetsPerFrame;
                    lc3_codec_config.sampleWidthInBits = codec_config_ptr->sampleWidthInBits;

                    if (!lc3_codec_initializeDecoder(LC3_DECODER_LEFT_CHANNEL_INDEX,
                                                     &lc3_codec_config))
                    {
                        printf(">> [%s]: LC3_DECODER_INIT: Decoder init failed!\n",
                               __FUNCTION__);
                        CY_ASSERT(0);
                    }

                    right_channel_enabled = false;

#ifndef AUDIO_CONFIG_MONO
                    /* If stereo config is enabled, initialize the decoder for right channel. */
                    if (codec_config_ptr->channelCount >= 2)
                    {
                        if (!lc3_codec_initializeDecoder(LC3_DECODER_RIGHT_CHANNEL_INDEX,
                                                         &lc3_codec_config))
                        {
                            printf(">> [%s]: LC3_DECODER_INIT: Decoder init failed!\n",
                                   __FUNCTION__);
                            CY_ASSERT(0);
                        }
                        right_channel_enabled = true;
                    }
#endif /* AUDIO_CONFIG_MONO */

                    /* Set the flags and variables appropriately. */
                    decoder_initialized = true;
                    lc3_encoded_data_buffer_index = 0;
                    pcm_data_buffer_index = 0;
                    printf("LC3 decoder ready for audio playback!\n");
#else
                    /* Set the flags and variables appropriately for raw data mode. */
                    decoder_initialized = true;
                    lc3_encoded_data_buffer_index = 0;
                    reception_reported = false;
                    printf("Ready to receive broadcast audio (LC3 decoding disabled; no playback).\n");
#endif /* ENABLE_LC3_DECODING */
                    break;
                }

                case LC3_DECODER_FREE:
                {
#if ENABLE_LC3_DECODING
                    if (!decoder_initialized)
                    {
                        printf(">> [%s]: LC3_DECODER_FREE: Decoder not initialized!\n",
                               __FUNCTION__);
                        break;
                    }

                    /* De-initialize the decoder. */
                    lc3_codec_releaseDecoder(LC3_DECODER_LEFT_CHANNEL_INDEX);

                    if (right_channel_enabled)
                    {
                        lc3_codec_releaseDecoder(LC3_DECODER_RIGHT_CHANNEL_INDEX);
                    }

                    /* Set the flags and variables appropriately. */
                    decoder_initialized = false;
                    right_channel_enabled = false;
                    lc3_encoded_data_buffer_index = 0;
                    pcm_data_buffer_index = 0;
#else
                    /* Set the flags and variables appropriately. */
                    decoder_initialized = false;
                    lc3_encoded_data_buffer_index = 0;
#endif /* ENABLE_LC3_DECODING */
                    xQueueReset(audio_decoder_task_q);

                    break;
                }

                case LC3_DECODER_RESET:
                {
#if ENABLE_LC3_DECODING
                    /* Reset the codec and variables. */
                    lc3_codec_reset();
                    lc3_encoded_data_buffer_index = 0;
                    pcm_data_buffer_index = 0;
#else
                    /* Reset the variables. */
                    lc3_encoded_data_buffer_index = 0;
#endif /* ENABLE_LC3_DECODING */
                    break;
                }

                case LC3_DECODER_DECODE_DATA:
                {
                    if (!decoder_initialized)
                    {
                        printf(">> [%s]: LC3_DECODER_DECODE_DATA: Decoder not initialized!\n",
                               __FUNCTION__);
                        break;
                    }

                    /* Assign the data pointer and sizes for decoding */
                    decoder_info_ptr = (lc3_decoder_info_t *) &audio_decoder_q_data.decoder.info;
                    decoded_data_size = decoder_info_ptr->expected_decoded_data_len_bytes;
                    lc3_data_l = NULL;
                    lc3_data_r = NULL;

#if ENABLE_LC3_DECODING
                    /* Obtain a pointer to store the PCM data post decoding. */
                    lc3_data_l = get_pcm_buffer_ptr(decoded_data_size);

                    if (lc3_data_l == NULL)
                    {
                        printf(">> [%s]: LC3_DECODER_DECODE_DATA: Memory allocation failed!\n",
                               __FUNCTION__);
                        break;
                    }

                    /* Perform LC3 decoding for the left channel data. */
                    lc3_codec_Decode(LC3_DECODER_LEFT_CHANNEL_INDEX,
                                     0,
                                     decoder_info_ptr->left_data_ptr,
                                     decoder_info_ptr->left_data_len_bytes,
                                     lc3_data_l,
                                     decoded_data_size);

                    /* If right channel is enabled, obtain another pointer for right channel
                     * PCM data and perform LC3 decoding.
                     */
                    if (right_channel_enabled && (decoder_info_ptr->right_data_len_bytes > 0))
                    {
                        lc3_data_r = get_pcm_buffer_ptr(decoded_data_size);

                        if (lc3_data_r == NULL)
                        {
                            printf(">> [%s]: LC3_DECODER_DECODE_DATA: Memory allocation failed!\n",
                                   __FUNCTION__);
                            break;
                        }

                        lc3_codec_Decode(LC3_DECODER_RIGHT_CHANNEL_INDEX,
                                         0,
                                         decoder_info_ptr->right_data_ptr,
                                         decoder_info_ptr->right_data_len_bytes,
                                         lc3_data_r,
                                         decoded_data_size);
                    }

                    /* Post decoding, send the PCM data (non interleaved) to
                     * the audio driver for playback on speaker.
                     */
                    audio_driver_write_non_interleaved_data(lc3_data_l,
                                                            lc3_data_r,
                                                            sizeof(int16_t),
                                                            decoded_data_size);
#else
                    /* Resolve compiler warnings */
                    (void) lc3_data_l;
                    (void) lc3_data_r;
                    (void) decoded_data_size;

                    /* LC3 decoding is disabled - just print raw data information */
                    total_bytes = decoder_info_ptr->left_data_len_bytes +
                                  decoder_info_ptr->right_data_len_bytes;

                    if (!reception_reported && total_bytes > 0)
                    {
                        printf("Receiving broadcast audio. LC3 decoding is disabled.\n");
                        reception_reported = true;
                    }
#endif /* ENABLE_LC3_DECODING */
                    break;
                }
            }
        }
    }
}

/******************************************************************************
* Function Name: get_lc3_buffer_ptr
*******************************************************************************
* Summary:
*   Function that returns the pointer to memory for LC3 data from the
*   available static circular buffer. This is similar to malloc but from a
*   static memory buffer.
*
* Parameters:
*   data_size_bytes : Size in bytes for the required buffer
*
* Return:
*   uint8_t* - Pointer output from the static buffer
*
*******************************************************************************/
uint8_t* get_lc3_buffer_ptr(uint32_t data_size_bytes)
{

    if (LC3_ENCODED_DATA_BUFFER_SIZE <= (lc3_encoded_data_buffer_index + data_size_bytes))
    {
        lc3_encoded_data_buffer_index = 0;
    }

    uint8_t *ptr = &(lc3_encoded_data_buffer[lc3_encoded_data_buffer_index]);
    lc3_encoded_data_buffer_index += data_size_bytes;

    return ptr;
}

/* [] END OF FILE */

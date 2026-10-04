/******************************************************************************
* File Name        : audio_driver_i2s_task.c
*
* Description      : This file contains the source code for the I2S audio
*                    driver on the CM55 CPU.
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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "audio_driver_i2s_task.h"
#include "audio_driver_psoc.h"
#include "cy_retarget_io.h"
#include "mtb_tlv320dac3100.h"
#include "wiced_bt_ga_bap.h"
#include "jitter_mgmt.h"

/******************************************************************************
* Macros
*******************************************************************************/

#define CHECK_RESULT(result)     do                                          \
                                 {                                           \
                                     CY_ASSERT(CY_RSLT_SUCCESS == result);   \
                                 }                                           \
                                 while(0)

/* Default volume setting for the Hardware Codec */
#define DEFAULT_VOLUME                      (100u)

#if (ENABLE_JITTER_MGMT_ASRC == 1)
/* Size of static buffer in bytes for PCM data post LC3 decoding, prior to
 * jitter management
 */
#define PCM_DATA_BUFFER_SIZE                (2 * DECODED_PCM_FRAME_2CH_SIZE)

/* Size of static buffer in bytes for PCM data post LC3 decoding and post
 * jitter management
 */
#define JITTER_MGMT_PCM_DATA_BUFFER_SIZE    (20 * DECODED_PCM_FRAME_1CH_SIZE)
#define JITTER_MGMT_PCM_EXTRA_BUFFER_SIZE   (DECODED_PCM_FRAME_1CH_SIZE)

#else
/* Size of static buffer in bytes for PCM data (post LC3 decoding) */
#define PCM_DATA_BUFFER_SIZE                (20 * DECODED_PCM_FRAME_2CH_SIZE)
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

/* Delay in ticks for I2S operations */
#define I2S_OPERATION_DELAY_TICKS           (3u)

#define I2S_QUEUE_WAIT_TIME_MS              (3u)
#define I2S_QUEUE_TX_SAMPLES_PER_CH         (HW_FIFO_SIZE / 2)

#define SYSCLK_DIV_CHANGE_WAIT_TIME_MS      (50u)

#define I2S_CLK_DIV_GRP_NUM                 (CYBSP_TDM_CONTROLLER_0_CLK_DIV_GRP_NUM)

/******************************************************************************
* Global Variables
*******************************************************************************/


/* Handle of the queue holding the I2S data. */
QueueHandle_t i2s_q;

/* I2S queue data */
static i2s_q_data_t i2s_q_data_rx;

/* I2S related flags */
static volatile bool i2s_q_init_complete = false;
static volatile bool i2s_init_complete = false;
static volatile bool i2s_tx_active = false;

/* I2C hal object and configuration used by the TLV codec library */
mtb_hal_i2c_t cybsp_i2c_controller_hal_obj;
cy_stc_scb_i2c_context_t cybsp_i2c_controller_context;
const mtb_hal_i2c_cfg_t cybsp_i2c_controller_config =
{
    .is_target = false,
    .address = I2C_ADDRESS,
    .frequency_hz = I2C_FREQUENCY_HZ,
    .address_mask = MTB_HAL_I2C_DEFAULT_ADDR_MASK,
    .enable_address_callback = false,
};

/* I2S transmit interrupt configurations */
const cy_stc_sysint_t i2s_isr_txcfg =
{
    .intrSrc = (IRQn_Type) tdm_0_interrupts_tx_0_IRQn,
    .intrPriority = I2S_INTERRUPT_PRIORITY,
};

/* Flag to maintain status of initial prefill for buffering. */
static volatile bool wait_for_prefill = true;

#if (ENABLE_JITTER_MGMT_ASRC == 1)
/* Static buffer for PCM data post LC3 decoding, prior to jitter management */
static uint8_t pcm_data_buffer[PCM_DATA_BUFFER_SIZE] = {0};
uint32_t pcm_data_buffer_index = 0;

/* Static buffers for PCM data post LC3 decoding, post jitter management */
static uint8_t  jitter_mgmt_left_pcm_data_buffer[JITTER_MGMT_PCM_DATA_BUFFER_SIZE + JITTER_MGMT_PCM_EXTRA_BUFFER_SIZE] = {0};
static uint32_t jitter_mgmt_left_pcm_data_buffer_index = 0;

static uint32_t jitter_mgmt_pcm_next_index_to_be_sent = 0;
static uint32_t jitter_mgmt_pcm_num_bytes_to_be_sent = 0;

static uint8_t  jitter_mgmt_right_pcm_data_buffer[JITTER_MGMT_PCM_DATA_BUFFER_SIZE + JITTER_MGMT_PCM_EXTRA_BUFFER_SIZE] = {0};
static uint32_t jitter_mgmt_right_pcm_data_buffer_index = 0;

#else
/* Static buffer for PCM data (post LC3 decoding) */
static uint8_t pcm_data_buffer[PCM_DATA_BUFFER_SIZE] = {0};
uint32_t pcm_data_buffer_index = 0;
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

/* Global variable to store the volume setting for the TLV hardware codec. */
static uint8_t tlv_codec_volume = DEFAULT_VOLUME;

/* Size of each frame of data in bytes for transmission over I2S */
static uint32_t i2s_data_bytes_per_ch = 0;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void start_i2s(void);
static void stop_i2s(void);

#if (ENABLE_JITTER_MGMT_ASRC == 1)
static void send_fixed_size_data_to_i2s(uint8_t *left_data_ptr,
                                        uint8_t *right_data_ptr,
                                        uint32_t data_size_per_channel);
static void send_queue_to_i2s_task(bool enable_right_channel);
#else
static void send_queue_to_i2s_task(uint8_t *left_data_ptr,
                                   uint8_t *right_data_ptr,
                                   uint32_t data_size_per_channel);
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

void app_i2s_init(void);
void app_i2s_deinit(void);
void app_i2s_enable(void);
void app_i2s_disable(void);
void app_i2s_activate(void);
void app_i2s_deactivate(void);
void i2s_tx_interrupt_handler(void);
void app_tlv_codec_init(uint32_t sampling_rate_hz);
void app_tlv_codec_deinit(void);
void configure_i2s_clocks(uint32_t sampling_rate_hz);
void tlv_codec_i2c_init(void);
void tlv_codec_i2c_deinit(void);

/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: start_i2s
*******************************************************************************
* Summary:
*   Function that enables and activates the I2S transmission (TX) if I2S block
*   is initialized and sets the I2S active flag to true.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
static void start_i2s(void)
{
    cy_rslt_t result = -1;
    (void) result;

    if (i2s_init_complete)
    {
        if (!i2s_tx_active)
        {
            app_i2s_enable();
            vTaskDelay(I2S_OPERATION_DELAY_TICKS);
            app_i2s_activate();
            i2s_tx_active = true;
        }
    }
}

/******************************************************************************
* Function Name: stop_i2s
*******************************************************************************
* Summary:
*   Function that stops and clears the I2S transmission (TX) and also clears
*   any existing data in the I2S queues and sets the I2S active flag to false.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
static void stop_i2s(void)
{
    cy_rslt_t result = -1;
    (void) result;

    if (i2s_init_complete)
    {
        if (i2s_tx_active)
        {
            app_i2s_deactivate();
            app_i2s_disable();

            i2s_tx_active = false;

            if (pdTRUE != xQueueReset(i2s_q))
            {
                CY_ASSERT(0);
            }
        }
    }
}

/******************************************************************************
* Function Name: get_pcm_buffer_ptr
*******************************************************************************
* Summary:
*   Function that returns the pointer to memory for PCM data from the
*   available static circular buffer. This is similar to malloc but from a
*   static memory buffer. This data is for PCM data post LC3 decoding and
*   prior to jitter management logic.
*
* Parameters:
*   data_size_bytes : Size in bytes for the required buffer
*
* Return:
*   uint8_t* - Pointer output from the static buffer
*
*******************************************************************************/
uint8_t* get_pcm_buffer_ptr(uint32_t data_size_bytes)
{
    uint8_t *ptr;
    /* If the buffer overflows for the required size, reset to start of the
     * buffer
     */
    if (PCM_DATA_BUFFER_SIZE <= (pcm_data_buffer_index + data_size_bytes))
    {
        pcm_data_buffer_index = 0;
    }

    /* Obtain the pointer and update the index for the circular buffer. */
    ptr = &(pcm_data_buffer[pcm_data_buffer_index]);
    pcm_data_buffer_index += data_size_bytes;

    return ptr;
}


#if (ENABLE_JITTER_MGMT_ASRC == 1)
/******************************************************************************
* Function Name: get_jitter_mgmt_left_pcm_buffer_ptr
*******************************************************************************
* Summary:
*   Function that returns the pointer to memory for PCM data from the
*   available static circular buffer. This is similar to malloc but from a
*   static memory buffer. This data is for PCM data post LC3 decoding and
*   post jitter management logic for the left channel audio.
*
* Parameters:
*   data_size_bytes : Size in bytes for the required buffer
*
* Return:
*   uint8_t* - Pointer output from the static buffer
*
*******************************************************************************/
uint8_t* get_jitter_mgmt_left_pcm_buffer_ptr(uint32_t data_size_bytes)
{
    uint8_t *ptr;
    /* If the buffer overflows for the required size, reset to start of the
     * buffer
     */
    if (JITTER_MGMT_PCM_DATA_BUFFER_SIZE <= (jitter_mgmt_left_pcm_data_buffer_index + data_size_bytes))
    {
        jitter_mgmt_left_pcm_data_buffer_index = 0;
    }

    /* Obtain the pointer and update the index for the circular buffer. */
    ptr = &(jitter_mgmt_left_pcm_data_buffer[jitter_mgmt_left_pcm_data_buffer_index]);
    jitter_mgmt_left_pcm_data_buffer_index += data_size_bytes;

    return ptr;
}

/******************************************************************************
* Function Name: get_jitter_mgmt_right_pcm_buffer_ptr
*******************************************************************************
* Summary:
*   Function that returns the pointer to memory for PCM data from the
*   available static circular buffer. This is similar to malloc but from a
*   static memory buffer. This data is for PCM data post LC3 decoding and
*   post jitter management logic for the right channel audio.
*
* Parameters:
*   data_size_bytes : Size in bytes for the required buffer
*
* Return:
*   uint8_t* - Pointer output from the static buffer
*
*******************************************************************************/
uint8_t* get_jitter_mgmt_right_pcm_buffer_ptr(uint32_t data_size_bytes)
{
    uint8_t *ptr;
    /* If the buffer overflows for the required size, reset to start of the
     * buffer
     */
    if (JITTER_MGMT_PCM_DATA_BUFFER_SIZE <= (jitter_mgmt_right_pcm_data_buffer_index + data_size_bytes))
    {
        jitter_mgmt_right_pcm_data_buffer_index = 0;
    }

    /* Obtain the pointer and update the index for the circular buffer. */
    ptr = &(jitter_mgmt_right_pcm_data_buffer[jitter_mgmt_right_pcm_data_buffer_index]);
    jitter_mgmt_right_pcm_data_buffer_index += data_size_bytes;

    return ptr;
}

#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */


/******************************************************************************
* Function Name: audio_driver_init
*******************************************************************************
* Summary:
*   Function that initializes the I2C, I2S, and the TLV320DAC3100 Hardware
*   Codec. It also sets up the I2S block for transmission.
*
* Parameters:
*   dir               : Direction of ISOC audio data
*   num_of_channels   : Number of audio channels
*   sample_rate       : Sampling rate of the PCM audio for the audio driver
*   frame_duration_us : Duration of each audio frame in microseconds (us)
*
* Return:
*   None
*
*******************************************************************************/
void audio_driver_init(wiced_ble_isoc_data_path_bit_t dir, uint8_t num_of_channels,
                       uint32_t sample_rate, uint32_t frame_duration_us)
{
    /* Create an RTOS queue for I2S if not created already */
    if (!i2s_q_init_complete)
    {
        i2s_q = xQueueCreate(I2S_QUEUE_LENGTH, sizeof(i2s_q_data_t));
        i2s_q_init_complete = true;
    }

    printf("\n  [%s]: Initializing Audio Driver for Channels = %d, Sampling" \
           " rate = %lu\n\n", __FUNCTION__, num_of_channels,
           (unsigned long) sample_rate);

    if (i2s_init_complete)
    {
        return;
    }

    /* Check if ISOC audio direction is "output" before initializing. */
    if (dir & WICED_BLE_ISOC_DPD_OUTPUT_BIT)
    {
        i2s_data_bytes_per_ch = I2S_QUEUE_TX_SAMPLES_PER_CH;
        i2s_data_bytes_per_ch *= sizeof(int16_t);

        printf("  [%s]: Frame duration = %lu us, Frame size = %lu bytes\n\n",
               __FUNCTION__, (unsigned long) frame_duration_us,
               (unsigned long) wiced_bt_ga_bap_get_decoded_data_size(
               sample_rate, frame_duration_us));

        /* I2S initialization */
        app_i2s_init();

        /* TLV codec initialization */
        app_tlv_codec_init(sample_rate);

        /* Set the appropriate flags */
        i2s_init_complete = true;
        wait_for_prefill = true;

        /* Set the volume and enable the Hardware Codec. */
        printf("\n  [%s]: Set default volume -> %d / 127\n", __FUNCTION__,
                                                             tlv_codec_volume);
        mtb_tlv320dac3100_adjust_speaker_output_volume(tlv_codec_volume);
        audio_driver_set_mute_state((uint8_t) audio_player_mute);

        /* Enable I2S TX transmission. */
        start_i2s();

#if (ENABLE_JITTER_MGMT_ASRC == 1)
        /* Reset all buffer indices. */
        jitter_mgmt_left_pcm_data_buffer_index = 0;
        jitter_mgmt_right_pcm_data_buffer_index = 0;
        jitter_mgmt_pcm_next_index_to_be_sent = 0;
        jitter_mgmt_pcm_num_bytes_to_be_sent = 0;

        jitter_mgmt_init(num_of_channels, sample_rate, frame_duration_us,
                         i2s_data_bytes_per_ch);
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

    }
}

/******************************************************************************
* Function Name: audio_driver_deinit
*******************************************************************************
* Summary:
*   Function that deinitializes the I2C, I2S, and the TLV320DAC3100 Hardware
*   Codec.
*
* Parameters:
*   dir  : Direction of ISOC audio data
*
* Return:
*   None
*
*******************************************************************************/
void audio_driver_deinit(wiced_ble_isoc_data_path_bit_t dir)
{
    printf("\n  [%s]: Deinitializing Audio Driver...\n\n", __FUNCTION__);

    if (!i2s_init_complete)
    {
        return;
    }

    /* Deinitialize playback for ISO audio received from the controller. */
    if (dir & WICED_BLE_ISOC_DPD_OUTPUT_BIT)
    {
        /* Stop any ongoing I2S transmission */
        stop_i2s();

        /* TLV codec de-initialization */
        app_tlv_codec_deinit();

        /* I2S de-initialization */
        app_i2s_deinit();

        /* Reset the appropriate flags */
        i2s_init_complete = false;
        wait_for_prefill = false;

        /* Remove any residue from I2S task queue. */
        xQueueReset(i2s_q);
    }
}

/******************************************************************************
* Function Name: audio_driver_write_non_interleaved_data
*******************************************************************************
* Summary:
*   Function that sends the non-interleaved data pointers over an RTOS
*   queue for transmission.
*
* Parameters:
*   p_left_data         : Pointer to the left channel data
*   p_right_data        : Pointer to the right channel data
*   bit_width_in_bytes  : Number of bytes per sample (sample bit-width / 8)
*   data_size           : Total number of bytes to be transmitted per channel
*
* Return:
*   None
*
*******************************************************************************/
void audio_driver_write_non_interleaved_data(uint8_t *p_left_data,
                                             uint8_t *p_right_data,
                                             uint8_t  bit_width_in_bytes,
                                             uint32_t data_size)
{
    if (bit_width_in_bytes != 2)
    {
        printf(">> Unsupported bit_width_in_bytes '%d'!!\n\n", bit_width_in_bytes);
        return;
    }

#if (ENABLE_JITTER_MGMT_ASRC == 1)
    uint8_t *post_jitter_left_data_ptr;
    uint8_t *post_jitter_right_data_ptr = NULL;
    jitter_mgmt_data_t jitter_mgmt_data[JITTER_MGMT_ASRC_MAX_CHANNELS];
    uint32_t post_jitter_buffer_size = 0;
    uint32_t num_channels = 1;

    /* Obtain a pointer for left channel jitter processing. */
    post_jitter_left_data_ptr = get_jitter_mgmt_left_pcm_buffer_ptr(data_size);

    jitter_mgmt_data[0].input_data_ptr = p_left_data;
    jitter_mgmt_data[0].output_data_ptr = (uint8_t *) post_jitter_left_data_ptr;
    (void) post_jitter_right_data_ptr;

    /* Perform the same operations for right channel if not NULL. */
    if (p_right_data != NULL)
    {
        num_channels ++;
        post_jitter_right_data_ptr = get_jitter_mgmt_right_pcm_buffer_ptr(data_size);

        jitter_mgmt_data[1].input_data_ptr = p_right_data;
        jitter_mgmt_data[1].output_data_ptr = (uint8_t *) post_jitter_right_data_ptr;
    }

    /* Perform jitter processing using ASRC algorithm. */
    post_jitter_buffer_size = jitter_mgmt_process_data(jitter_mgmt_data,
                                                       num_channels,
                                                       data_size);

    /* Update the buffer indices as per actual number of samples used post ASRC. */
    if (post_jitter_buffer_size != data_size)
    {
        jitter_mgmt_left_pcm_data_buffer_index += ((int32_t) post_jitter_buffer_size -
                                                   (int32_t) data_size);
        if (p_right_data != NULL)
        {
            jitter_mgmt_right_pcm_data_buffer_index += ((int32_t) post_jitter_buffer_size -
                                                        (int32_t) data_size);
        }
    }

    /* Send the data pointers and size to the I2S task. */
    send_fixed_size_data_to_i2s(post_jitter_left_data_ptr,
                                post_jitter_right_data_ptr,
                                post_jitter_buffer_size);
#else
    /* Send the data pointers and size to the I2S task. */
    send_queue_to_i2s_task(p_left_data, p_right_data, data_size);
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */
}

#if (ENABLE_JITTER_MGMT_ASRC == 1)
/******************************************************************************
* Function Name: send_fixed_size_data_to_i2s
*******************************************************************************
* Summary:
*   Function that queues data pointers in fixed-size frames for I2S playback.
*
*   This function takes in the pointers for the left and right channels audio
*   from jitter_mgmt_left_pcm_data_buffer and jitter_mgmt_right_pcm_data_buffer
*   buffers respectively. The size of data ready to be sent to I2S could vary.
*   But this function ensures only a fixed size of data is sent to the I2S
*   task by maintaining appropriate pointers and states.
*
* Parameters:
*   left_data_ptr         : Pointer to the left channel data to be sent
*   right_data_ptr        : Pointer to the right channel data to be sent
*   data_size_per_channel : Size of the data to be sent in bytes per channel
*
* Return:
*   None
*
*******************************************************************************/
static void send_fixed_size_data_to_i2s(uint8_t *left_data_ptr,
                                        uint8_t *right_data_ptr,
                                        uint32_t data_size_per_channel)
{

    int32_t left_index, left_curr_data_size;
    int32_t bytes_to_be_copied = 0;

    if (data_size_per_channel == 0)
    {
        return;
    }

    /* Obtain the buffer level using the left channel data as a reference. */
    left_index = left_data_ptr - jitter_mgmt_left_pcm_data_buffer;
    left_curr_data_size = left_index + data_size_per_channel -
                           jitter_mgmt_pcm_next_index_to_be_sent;

    /* If there is data to be sent directly to I2S task, send the pointers
     * and the size without any memory copying.
     */
    if (left_curr_data_size > 0)
    {
        jitter_mgmt_pcm_num_bytes_to_be_sent += data_size_per_channel;
        send_queue_to_i2s_task(NULL != right_data_ptr);
    }
    else
    {
        /* Handle roll over case */
        send_queue_to_i2s_task(NULL != right_data_ptr);
        if (jitter_mgmt_pcm_num_bytes_to_be_sent > 0)
        {
            bytes_to_be_copied = i2s_data_bytes_per_ch - jitter_mgmt_pcm_num_bytes_to_be_sent;
            if (bytes_to_be_copied > data_size_per_channel)
            {
                bytes_to_be_copied = data_size_per_channel;
            }

            memcpy(&(jitter_mgmt_left_pcm_data_buffer[jitter_mgmt_pcm_next_index_to_be_sent
                     + jitter_mgmt_pcm_num_bytes_to_be_sent]), left_data_ptr, bytes_to_be_copied);

            if (NULL != right_data_ptr)
            {
                memcpy(&(jitter_mgmt_right_pcm_data_buffer[jitter_mgmt_pcm_next_index_to_be_sent
                         + jitter_mgmt_pcm_num_bytes_to_be_sent]), right_data_ptr, bytes_to_be_copied);
            }

            jitter_mgmt_pcm_num_bytes_to_be_sent += bytes_to_be_copied;
            send_queue_to_i2s_task(NULL != right_data_ptr);

            if (jitter_mgmt_pcm_num_bytes_to_be_sent != 0)
            {
                printf(">> Error: Jitter Buffer Management Failed !!\n");
                CY_ASSERT(0);
            }

            jitter_mgmt_pcm_num_bytes_to_be_sent = data_size_per_channel - bytes_to_be_copied;
            jitter_mgmt_pcm_next_index_to_be_sent = left_index + bytes_to_be_copied;
        }
        else
        {
            jitter_mgmt_pcm_num_bytes_to_be_sent += data_size_per_channel;
            jitter_mgmt_pcm_next_index_to_be_sent = left_index;
            send_queue_to_i2s_task(NULL != right_data_ptr);
        }
    }
}
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

/******************************************************************************
* Function Name: send_queue_to_i2s_task
*******************************************************************************
* Summary:
*   Function that sends the pointers and size to I2S task over an RTOS queue.
*
* Parameters:
*   With ASRC enabled:
*   enable_right_channel  : Flag to enable right channel audio
*   With ASRC disabled:
*   left_data_ptr         : Pointer to the left channel data
*   right_data_ptr        : Pointer to the right channel data, or NULL for mono
*   data_size_per_channel : Data size in bytes per channel
*
* Return:
*   None
*
*******************************************************************************/
#if (ENABLE_JITTER_MGMT_ASRC == 1)
static void send_queue_to_i2s_task(bool enable_right_channel)
#else
static void send_queue_to_i2s_task(uint8_t *left_data_ptr,
                                   uint8_t *right_data_ptr,
                                   uint32_t data_size_per_channel)
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */
{
    i2s_q_data_t i2s_q_data_tx;

#if (ENABLE_JITTER_MGMT_ASRC == 1)
    /* Queue complete frames while the available byte count is at least the
     * fixed frame size.
     */
    while (jitter_mgmt_pcm_num_bytes_to_be_sent >= i2s_data_bytes_per_ch)
    {
        /* Assign the pointers and size in the queue data structure. */
        i2s_q_data_tx.left_data = &(jitter_mgmt_left_pcm_data_buffer[
                                    jitter_mgmt_pcm_next_index_to_be_sent]);
        i2s_q_data_tx.right_data = &(jitter_mgmt_right_pcm_data_buffer[
                                     jitter_mgmt_pcm_next_index_to_be_sent]);
        i2s_q_data_tx.data_len_bytes_per_ch = i2s_data_bytes_per_ch;

        /* Set the buffer indices appropriately. */
        jitter_mgmt_pcm_num_bytes_to_be_sent -= i2s_data_bytes_per_ch;
        jitter_mgmt_pcm_next_index_to_be_sent += i2s_data_bytes_per_ch;

        /* Set the right channel pointer to NULL if not enabled. */
        if (!enable_right_channel)
        {
            i2s_q_data_tx.right_data = NULL;
        }

        /* Update the buffer levels for the jitter management algorithm. */
        jitter_mgmt_update_buffer_level((int32_t)i2s_data_bytes_per_ch / sizeof(int16_t));

        /* Send the queue data to the I2S task. */
        if (pdTRUE != xQueueSend(i2s_q, &i2s_q_data_tx, portMAX_DELAY))
        {
            printf(">> Error: I2S Task Queue Send failed !!\n");
            CY_ASSERT(0);
        }
    }
#else
    /* Send the pointers and size to the I2S task over RTOS queue. */
    i2s_q_data_tx.data_len_bytes_per_ch = data_size_per_channel;

    i2s_q_data_tx.left_data = left_data_ptr;
    i2s_q_data_tx.right_data = right_data_ptr;

    if (pdTRUE != xQueueSend(i2s_q, &i2s_q_data_tx, portMAX_DELAY))
    {
        printf(">> Error: I2S Task Queue Send failed !!\n");
        CY_ASSERT(0);
    }
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */
}


/******************************************************************************
* Function Name: audio_driver_set_volume
*******************************************************************************
* Summary:
*   Function that sets the desired volume in the TLV320DAC3100 Hardware Codec.
*
* Parameters:
*   volume  : Desired volume value as an integer (Valid range: 0 - 100)
*
* Return:
*   None
*
*******************************************************************************/
void audio_driver_set_volume(uint8_t volume)
{
    /* Range of 'volume' is 0-100. Converting it to 0-127 for the TLV codec. */
    uint32_t scaled_volume = (volume * 0x7F) / 100u;
    tlv_codec_volume = (uint8_t) scaled_volume;
    printf("\n  [%s]: Set volume -> %d / 127\n", __FUNCTION__, tlv_codec_volume);

    if (!i2s_init_complete)
    {
        return;
    }

    mtb_tlv320dac3100_adjust_speaker_output_volume(tlv_codec_volume);
}

/******************************************************************************
* Function Name: audio_driver_set_mute_state
*******************************************************************************
* Summary:
*   Function that sets the desired mute state in TLV320DAC3100 Hardware Codec.
*
* Parameters:
*   mute_enabled : Desired mute state (0 -> Unmute, 1 -> Mute)
*
* Return:
*   None
*
*******************************************************************************/
void audio_driver_set_mute_state(uint8_t mute_enabled)
{
    printf("\n  [%s]: Set Mute Status -> %d\n", __FUNCTION__, mute_enabled);
    audio_player_mute = (0 != mute_enabled);

    if (!i2s_init_complete)
    {
        return;
    }

    /* Set the appropriate value in the DAC Volume Control register */
    if (0u == mute_enabled)
    {
        mtb_tlv320dac3100_write_byte(TLV320DAC3100_P0_REG_DAC_VOL_CTL, 0x00);
    }
    else
    {
        mtb_tlv320dac3100_write_byte(TLV320DAC3100_P0_REG_DAC_VOL_CTL, 0x0C);
    }
}

/******************************************************************************
* Function Name: app_i2s_init
*******************************************************************************
* Summary:
*   Initialize I2S interrupt and I2S (TDM) block.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_i2s_init(void)
{
    /* Initialize the I2S interrupt */
    Cy_SysInt_Init(&i2s_isr_txcfg, i2s_tx_interrupt_handler);

    /* Initialize the I2S */
    cy_en_tdm_status_t return_status = Cy_AudioTDM_Init(TDM_STRUCT0, &CYBSP_TDM_CONTROLLER_0_config);
    if (CY_TDM_SUCCESS != return_status)
    {
        CY_ASSERT(0);
    }
}

/******************************************************************************
* Function Name: app_i2s_deinit
*******************************************************************************
* Summary:
*   De-initialize I2S (TDM) block.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_i2s_deinit(void)
{
    /* De-initialize the I2S */
    Cy_AudioTDM_DeInit(TDM_STRUCT0);
}

/******************************************************************************
* Function Name: app_tlv_codec_init
*******************************************************************************
* Summary:
*   Initializes the I2C and TLV codec.
*
* Parameters:
*   sampling_rate_hz: Sampling rate in Hz
*
* Return:
*   None
*
*******************************************************************************/
void app_tlv_codec_init(uint32_t sampling_rate_hz)
{
    /* Initialize I2C used to configure TLV codec */
    tlv_codec_i2c_init();
    /* TLV codec (TLV320DAC3100) library */
    mtb_tlv320dac3100_init(&cybsp_i2c_controller_hal_obj);
    /* Configure internal clock dividers to achieve desired sample rate */
    configure_i2s_clocks(sampling_rate_hz);
    /* Activate TLV codec (TLV320DAC3100) */
    mtb_tlv320dac3100_activate();
}

/******************************************************************************
* Function Name: app_tlv_codec_deinit
*******************************************************************************
* Summary:
*   Deinitializes the I2C and TLV codec.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_tlv_codec_deinit(void)
{
    mtb_tlv320dac3100_deactivate();
    mtb_tlv320dac3100_free();
    tlv_codec_i2c_deinit();
}

/******************************************************************************
* Function Name: configure_i2s_clocks
*******************************************************************************
* Summary:
*   Configure I2S clocks based on the sampling rate.
*
* Parameters:
*   sampling_rate_hz: Sampling rate in Hz
*
* Return:
*   None
*
*******************************************************************************/
void configure_i2s_clocks(uint32_t sampling_rate_hz)
{
    uint32_t mclk_hz = 0;

    switch (sampling_rate_hz)
    {
        case I2S_SAMPLING_RATE_48K:
        {
            Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U);
            Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U,
                                             I2S0_CLK_DIV_48KHZ - 1, 0U);
            Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                            CY_SYSCLK_DIV_16_5_BIT, 0U);
            break;
        }
        case I2S_SAMPLING_RATE_24K:
        {
            Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U);
            Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U,
                                             I2S0_CLK_DIV_24KHZ - 1, 0U);
            Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U);
            break;
        }
        case I2S_SAMPLING_RATE_16K:
        {
            Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U);
            Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U,
                                             I2S0_CLK_DIV_16KHZ - 1, 0U);
            Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                            CY_SYSCLK_DIV_16_5_BIT, 0U);
            break;
        }
        case I2S_SAMPLING_RATE_8K:
        {
            Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U);
            Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U,
                                             I2S0_CLK_DIV_8KHZ - 1, 0U);
            Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                            CY_SYSCLK_DIV_16_5_BIT, 0U);
            break;
        }
        default:
        {
            printf(">> Error: Unsupported sampling rate %lu Hz\n",
                   (unsigned long) sampling_rate_hz);
            CY_ASSERT(0);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(SYSCLK_DIV_CHANGE_WAIT_TIME_MS));

    mclk_hz = Cy_SysClk_PeriPclkGetFrequency((en_clk_dst_t)I2S_CLK_DIV_GRP_NUM,
                                             CY_SYSCLK_DIV_16_5_BIT, 0U);

    /* Configure internal clock dividers to achieve desired sample rate */
    mtb_tlv320dac3100_configure_clocking(mclk_hz,
                                         (mtb_tlv320dac3100_dac_sample_rate_t) sampling_rate_hz,
                                         I2S_WORD_LENGTH, AUDIO_OUTPUT);
}

/******************************************************************************
* Function Name: i2s_tx_interrupt_handler
*******************************************************************************
* Summary:
*   I2S TX interrupt handler function.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void i2s_tx_interrupt_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int16_t* temp_left_i2s_tx_buffer;
    int16_t* temp_right_i2s_tx_buffer;
    uint32_t i2s_tx_num_samples = 0;
    uint32_t left_ch_data = 0;
    uint32_t right_ch_data = 0;

    /* Get interrupt status and check for trigger interrupt and errors */
    uint32_t intr_status = Cy_AudioTDM_GetTxInterruptStatusMasked(TDM_STRUCT0_TX);

    if(CY_TDM_INTR_TX_FIFO_TRIGGER & intr_status)
    {
        /* If prefill is enabled, wait until the required number of queue items
         * are present before first playback.
         */
        if (wait_for_prefill)
        {
            if (I2S_TX_INITIAL_FRAME_DELAY > uxQueueMessagesWaitingFromISR(i2s_q))
            {
                for (uint32_t i = 0; i < (HW_FIFO_SIZE / 2); i++)
                {
                    Cy_AudioTDM_WriteTxData(TDM_STRUCT0_TX, 0UL);
                    Cy_AudioTDM_WriteTxData(TDM_STRUCT0_TX, 0UL);
                }
            }
            else
            {
                wait_for_prefill = false;
            }
        }
        else
        {
            /* Receive the data from the queue and process it. */
            if (pdTRUE == xQueueReceiveFromISR(i2s_q, &i2s_q_data_rx,
                                               &xHigherPriorityTaskWoken))
            {
                temp_left_i2s_tx_buffer = (int16_t *) i2s_q_data_rx.left_data;
                temp_right_i2s_tx_buffer = (int16_t *) i2s_q_data_rx.right_data;
                i2s_tx_num_samples = (i2s_q_data_rx.data_len_bytes_per_ch /
                                      sizeof(int16_t));

            #if (ENABLE_JITTER_MGMT_ASRC == 1)
                jitter_mgmt_update_buffer_level(-((int32_t)i2s_tx_num_samples));
                jitter_mgmt_update_asrc_state();
            #endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

                if ((HW_FIFO_SIZE / 2) != i2s_tx_num_samples)
                {
                    printf(">> Error: I2S TX data size mismatch !!\n");
                    CY_ASSERT(0);
                }

                for (uint32_t i = 0; i < (HW_FIFO_SIZE / 2); i++)
                {
                    left_ch_data = 0UL;
                    right_ch_data = 0UL;

                    if (temp_left_i2s_tx_buffer != NULL)
                    {
                        left_ch_data = (uint32_t) temp_left_i2s_tx_buffer[i];
                    }

                    if (temp_right_i2s_tx_buffer != NULL)
                    {
                        right_ch_data = (uint32_t) temp_right_i2s_tx_buffer[i];
                    }
                    else
                    {
                        right_ch_data = left_ch_data;
                    }

                    Cy_AudioTDM_WriteTxData(TDM_STRUCT0_TX, left_ch_data);
                    Cy_AudioTDM_WriteTxData(TDM_STRUCT0_TX, right_ch_data);
                }
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            else
            {
                for (uint32_t i = 0; i < (HW_FIFO_SIZE / 2); i++)
                {
                    Cy_AudioTDM_WriteTxData(TDM_STRUCT0_TX, 0UL);
                    Cy_AudioTDM_WriteTxData(TDM_STRUCT0_TX, 0UL);
                }
            }
        }
    }
    else if(CY_TDM_INTR_TX_FIFO_UNDERFLOW & intr_status)
    {
        printf(">> Error: I2S transmit underflowed");
    }

    /* Clear all TX I2S Interrupt */
    Cy_AudioTDM_ClearTxInterrupt(TDM_STRUCT0_TX, CY_TDM_INTR_TX_MASK);
}

/******************************************************************************
* Function Name: app_i2s_activate
*******************************************************************************
* Summary:
*   Activate I2S TX and the interrupts.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_i2s_activate(void)
{
    /* Activate and enable I2S TX interrupts */
    NVIC_EnableIRQ(i2s_isr_txcfg.intrSrc);
    Cy_AudioTDM_ActivateTx(TDM_STRUCT0_TX);
}


/******************************************************************************
* Function Name: app_i2s_deactivate
*******************************************************************************
* Summary:
*   Disable the I2S interrupt and clear TX FIFO and disable the I2S
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_i2s_deactivate(void)
{
    /* De-initialize the I2S interrupt */
    NVIC_DisableIRQ(i2s_isr_txcfg.intrSrc);

    /* There is no API to clear the HW FIFO. Disabling and enabling the TX
     * will clear the FIFO as a side effect.
     */
    Cy_AudioTDM_DeActivateTx(TDM_STRUCT0_TX);
    Cy_AudioI2S_DisableTx(TDM_STRUCT0_TX);
    Cy_AudioI2S_EnableTx(TDM_STRUCT0_TX);
    Cy_AudioTDM_DeActivateTx(TDM_STRUCT0_TX);
    Cy_AudioI2S_DisableTx(TDM_STRUCT0_TX);
}

/******************************************************************************
* Function Name: app_i2s_enable
*******************************************************************************
* Summary:
*   Enable I2S TX, its interrupts and fill the FIFO with zeros to start TX.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_i2s_enable(void)
{
    /* Enable the I2S TX interface */
    Cy_AudioTDM_EnableTx(TDM_STRUCT0_TX);

    /* Clear TX interrupts */
    Cy_AudioTDM_ClearTxInterrupt(TDM_STRUCT0_TX, CY_TDM_INTR_TX_MASK);
    Cy_AudioTDM_SetTxInterruptMask(TDM_STRUCT0_TX, CY_TDM_INTR_TX_MASK);

    /* Fill TX FIFO with zeros to start the transmission */
    for(uint32_t i = 0; i < (HW_FIFO_SIZE / 2); i++)
    {
        /* Write data in FIFO */
        Cy_AudioTDM_WriteTxData(TDM0_TDM_STRUCT0_TDM_TX_STRUCT, 0UL);
        Cy_AudioTDM_WriteTxData(TDM0_TDM_STRUCT0_TDM_TX_STRUCT, 0UL);
    }
}

/******************************************************************************
* Function Name: app_i2s_disable
*******************************************************************************
* Summary:
*   Disable I2S TX and interrupts.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void app_i2s_disable(void)
{
    /* Disable the I2S TX interface */
    Cy_AudioTDM_DisableTx(TDM_STRUCT0_TX);

    /* Clear TX interrupts */
    Cy_AudioTDM_ClearTxInterrupt(TDM_STRUCT0_TX, CY_TDM_INTR_TX_MASK);
    Cy_AudioTDM_ClearTxTriggerInterruptMask(TDM_STRUCT0_TX);
}

/******************************************************************************
* Function Name: tlv_codec_i2c_init
*******************************************************************************
* Summary:
*   Initialize the I2C for the TLV codec.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void tlv_codec_i2c_init(void)
{
    cy_en_scb_i2c_status_t result;
    cy_rslt_t hal_result;

    /* Initialize and enable the I2C in master mode. */
    result = Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW, &CYBSP_I2C_CONTROLLER_config, &cybsp_i2c_controller_context);
    if(result != CY_SCB_I2C_SUCCESS)
    {
        CY_ASSERT(0);
    }
    /* Enable I2C master hardware. */
    Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);

    /* I2C HAL init */
    hal_result = mtb_hal_i2c_setup(&cybsp_i2c_controller_hal_obj, &CYBSP_I2C_CONTROLLER_hal_config,
                                   &cybsp_i2c_controller_context, NULL);
    if (CY_RSLT_SUCCESS != hal_result)
    {
        CY_ASSERT(0);
    }

    /* Configure the I2C block */
    hal_result = mtb_hal_i2c_configure(&cybsp_i2c_controller_hal_obj, &cybsp_i2c_controller_config);
    if (CY_RSLT_SUCCESS != hal_result)
    {
        CY_ASSERT(0);
    }
}

/******************************************************************************
* Function Name: tlv_codec_i2c_deinit
*******************************************************************************
* Summary:
*   Deinitialize the I2C used by the TLV codec.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void tlv_codec_i2c_deinit(void)
{
    /* Disable and deinitialize the I2C resource */
    Cy_SCB_I2C_Disable(CYBSP_I2C_CONTROLLER_HW, &cybsp_i2c_controller_context);
    Cy_SCB_I2C_DeInit(CYBSP_I2C_CONTROLLER_HW);
}


/* [] END OF FILE */

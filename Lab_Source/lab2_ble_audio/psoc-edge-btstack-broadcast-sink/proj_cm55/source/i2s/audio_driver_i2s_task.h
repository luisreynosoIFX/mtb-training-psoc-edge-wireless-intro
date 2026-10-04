/******************************************************************************
* File Name        : audio_driver_i2s_task.h
*
* Description      : This file is the public interface of audio_driver_i2s_task.c.
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

#ifndef I2S_TASK_H
#define I2S_TASK_H

/******************************************************************************
* Header Files
*******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include "jitter_mgmt.h"

/******************************************************************************
* Macros
*******************************************************************************/
/* I2S Queue Length configuration */
#define I2S_QUEUE_LENGTH                            (450u)

/* I2S Sampling rate macros */
#define I2S_SAMPLING_RATE_8K                        (8000UL)
#define I2S_SAMPLING_RATE_16K                       (16000UL)
#define I2S_SAMPLING_RATE_24K                       (24000UL)
#define I2S_SAMPLING_RATE_32K                       (32000UL)
#define I2S_SAMPLING_RATE_48K                       (48000UL)

/* Pre-fill count in frames for I2S playback buffer */
#define I2S_TX_INITIAL_FRAME_DELAY                  (75u)

/* Size of decoded PCM frame in bytes */
#define DECODED_PCM_FRAME_2CH_SIZE                  (480 * 2 * sizeof(int16_t))
#define DECODED_PCM_FRAME_1CH_SIZE                  (480 * sizeof(int16_t))

/* Priority for I2S block interrupt */
#define I2S_INTERRUPT_PRIORITY                      (3u)

/* I2S word length parameter */
#define I2S_WORD_LENGTH                             (TLV320DAC3100_I2S_WORD_SIZE_16)

/* I2S hardware FIFO size */
#define HW_FIFO_SIZE                                (64u)

/* I2C master address */
#define I2C_ADDRESS                                 (0x18)

/* I2C frequency in Hz */
#if (defined(COMPONENT_ARM) || defined(COMPONENT_LLVM_ARM)) && defined(AUDIO_CONFIG_MONO)
    #define I2C_FREQUENCY_HZ                        (100000u)
#else
    #define I2C_FREQUENCY_HZ                        (400000u)
#endif

/* I2S clock divider values for different sampling rates */
#define I2S0_CLK_DIV_48KHZ                          (2u)
#define I2S0_CLK_DIV_24KHZ                          (4u)
#define I2S0_CLK_DIV_16KHZ                          (6u)
#define I2S0_CLK_DIV_8KHZ                           (12u)

/* Audio output configuration:
 *  On-board speaker : TLV320DAC3100_SPK_AUDIO_OUTPUT
 *  Headphone jack   : TLV320DAC3100_HP_AUDIO_OUTPUT
 */
#define AUDIO_OUTPUT                                (TLV320DAC3100_SPK_AUDIO_OUTPUT)


/******************************************************************************
* Data structures and enumeration
*******************************************************************************/
/* Data-type for I2S queue data */
typedef struct
{
    /* Total number of bytes of data to be transmitted via I2S per channel */
    uint32_t data_len_bytes_per_ch;

    /* Pointer to the start of the data to be transmitted */
    uint8_t* left_data;

    /* Pointer to the start of the data to be transmitted */
    uint8_t* right_data;
} i2s_q_data_t;


/******************************************************************************
* Extern Variables
*******************************************************************************/
extern QueueHandle_t i2s_q;
extern uint32_t pcm_data_buffer_index;


/******************************************************************************
* Function Prototypes
*******************************************************************************/
uint8_t* get_pcm_buffer_ptr(uint32_t data_size_bytes);

#if (ENABLE_JITTER_MGMT_ASRC == 1)
uint8_t* get_jitter_mgmt_left_pcm_buffer_ptr(uint32_t data_size_bytes);
uint8_t* get_jitter_mgmt_right_pcm_buffer_ptr(uint32_t data_size_bytes);
#endif /* (ENABLE_JITTER_MGMT_ASRC == 1) */

#endif  /* I2S_TASK_H */

/* [] END OF FILE */

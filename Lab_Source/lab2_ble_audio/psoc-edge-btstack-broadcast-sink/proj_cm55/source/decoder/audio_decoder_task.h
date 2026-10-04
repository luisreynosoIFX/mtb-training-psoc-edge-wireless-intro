/******************************************************************************
* File Name        : audio_decoder_task.h
*
* Description      : This file is the public interface of audio_decoder_task.c.
*                    It also contains the decoder task configuration parameters.
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

#ifndef AUDIO_DEC_TASK_H_
#define AUDIO_DEC_TASK_H_

/******************************************************************************
* Header Files
*******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cy_pdl.h"
#include "cybsp.h"

/******************************************************************************
* Macros
*******************************************************************************/
/* Task priority and stack size for the Audio Decoder task */
#define AUDIO_DECODER_TASK_PRIORITY                 (4u)
#define AUDIO_DECODER_TASK_STACK_SIZE               (1 * 1024u)

/* Maximum number of elements in the Audio Decoder task queue */
#define AUDIO_DECODER_TASK_QUEUE_LENGTH             (30u)


/******************************************************************************
* Data structures and enumeration
*******************************************************************************/
/* Data structures to store LC3 decoder configurations and information */
typedef struct
{
    uint16_t sampleRate;
    uint16_t sduInterval;
    uint16_t octetsPerFrame;
    uint8_t sampleWidthInBits;
    uint8_t channelCount;
} lc3_decoder_config_t;

typedef struct
{
    uint8_t* left_data_ptr;
    uint32_t left_data_len_bytes;
    uint8_t* right_data_ptr;
    uint32_t right_data_len_bytes;
    uint32_t expected_decoded_data_len_bytes;
} lc3_decoder_info_t;

typedef union
{
    lc3_decoder_config_t config;
    lc3_decoder_info_t info;
} lc3_decoder_union_t;

/* Data-type for Audio Decoder task commands */
typedef enum
{
    LC3_DECODER_INIT,
    LC3_DECODER_FREE,
    LC3_DECODER_RESET,
    LC3_DECODER_DECODE_DATA,
} audio_decoder_cmd_t;

/* Data-type for Audio Decoder task queue data */
typedef struct
{
    audio_decoder_cmd_t cmd;
    lc3_decoder_union_t decoder;
} audio_decoder_q_data_t;


/******************************************************************************
* Extern Variables
*******************************************************************************/
extern TaskHandle_t audio_decoder_task_handle;
extern QueueHandle_t audio_decoder_task_q;


/******************************************************************************
* Function Prototypes
*******************************************************************************/
cy_rslt_t create_audio_decoder_task(void);
uint8_t* get_lc3_buffer_ptr(uint32_t data_size_bytes);

#endif /* AUDIO_DEC_TASK_H_ */

/* [] END OF FILE */

/******************************************************************************
* File Name        : jitter_mgmt.h
*
* Description      : This file is the public interface of jitter_mgmt.c.
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

#ifndef JITTER_MGMT_H
#define JITTER_MGMT_H

/******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"

/******************************************************************************
* Macros
*******************************************************************************/
#define ENABLE_JITTER_MGMT_ASRC                 (1u)
#define JITTER_MGMT_ASRC_MAX_CHANNELS           (2u)
#define JITTER_MGMT_ASRC_MAX_VARIATION_SAMPLES  (2u)
#define JITTER_MGMT_ASRC_MAX_FRAME_BUFFER_SIZE  (DECODED_PCM_FRAME_1CH_SIZE)
#define JITTER_MGMT_ASRC_OUTPUT_BUFFER_SIZE     (1000u) /* 964 as per ASRC lib */
#define JITTER_MGMT_WATERMARK_MONITOR_COUNT_US  (1500L)

extern bool audio_player_mute;
/******************************************************************************
* Data structures and enumeration
*******************************************************************************/
/* Structure that stores input and output data pointers for jitter management. */
typedef struct
{
    /* Pointer to the input data for jitter management processing */
    uint8_t* input_data_ptr;

    /* Pointer to the output data for jitter management processing */
    uint8_t* output_data_ptr;
} jitter_mgmt_data_t;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
void jitter_mgmt_init(uint32_t num_channels, uint32_t sampling_rate_hz,
                      uint32_t frame_duration_us, uint32_t frame_size_bytes);
uint32_t jitter_mgmt_process_data(jitter_mgmt_data_t *jitter_mgmt_data,
                                  uint32_t num_channels,
                                  uint32_t data_size_per_channel);
void jitter_mgmt_update_buffer_level(int32_t num_samples);
void jitter_mgmt_update_asrc_state(void);

#endif  /* JITTER_MGMT_H */

/* [] END OF FILE */

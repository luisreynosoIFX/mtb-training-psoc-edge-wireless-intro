/******************************************************************************
* File Name        : broadcast_source.h
*
* Description      : Public interface for Broadcast Source application control.
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

#ifndef BROADCAST_SOURCE_H
#define BROADCAST_SOURCE_H

/*******************************************************************************
* Header Files
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define MAX_BROADCASTCODE_LEN 16
#define MAX_BROADCAST_ID  999999

/*******************************************************************************
* Data Types
*******************************************************************************/
typedef struct
{
    uint8_t start;
    uint32_t codec_config;
    uint8_t enable_encryption;
    uint32_t channel_counts;
    uint32_t broadcast_id;
    uint8_t broadcast_code[MAX_BROADCASTCODE_LEN];
    uint8_t bis_count;
} broadcast_stream_config;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void broadcast_source_init_and_start(void);
void print_broadcast_stream_config(broadcast_stream_config config);
void broadcast_source_handle_start_streaming(broadcast_stream_config config);
bool is_broadcast_streaming(broadcast_stream_config config);
void set_broadcast_stream_config_codec(uint32_t codec, broadcast_stream_config *config);
void set_broadcast_stream_config_encryption(bool enable, broadcast_stream_config *config);
void set_broadcast_stream_config_channelcounts(uint32_t channel_counts, broadcast_stream_config *config);
void set_broadcast_stream_config_broadcastid(uint32_t broadcast_id, broadcast_stream_config *config);
void set_broadcast_stream_config_broadcastcode(int8_t *broadcast_code, broadcast_stream_config *config);

#endif

/******************************************************************************
* File Name        : lc3_codec.h
*
* Description      : This file is the public interface of the lc3_codec.c
*                    source file.
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

#pragma once

/******************************************************************************
* Header Files
*******************************************************************************/
#include "wiced_data_types.h"

/******************************************************************************
* Macros
*******************************************************************************/
#define ISO_AUDIO_MAX_PARAM_COUNT   4 /* 2 channels per device for 2 devices */

/******************************************************************************
* Data structures and enumeration
*******************************************************************************/
typedef struct
{
    uint16_t sampleRate;
    uint16_t sduInterval;
    uint16_t octetsPerFrame;
    uint8_t sampleWidthInBits;
} lc3_config_t;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
#if ENABLE_LC3_DECODING
void lc3_codec_reset(void);
wiced_bool_t lc3_codec_initializeDecoder(uint8_t index, lc3_config_t *p_lc3Config);
void lc3_codec_releaseDecoder(uint8_t index);
uint32_t lc3_codec_Decode(uint8_t index, uint8_t pktStatus, void *inBuf,
                          uint16_t inLenBytes, void *outBuf, uint16_t outLenBytes);
#else
/* Stub functions when LC3 decoding is disabled */
static inline void lc3_codec_reset(void)
{
}

static inline wiced_bool_t lc3_codec_initializeDecoder(uint8_t index, lc3_config_t *p_lc3Config)
{
    (void)index;
    (void)p_lc3Config;
    return WICED_TRUE;
}

static inline void lc3_codec_releaseDecoder(uint8_t index)
{
    (void)index;
}

static inline uint32_t lc3_codec_Decode(uint8_t index, uint8_t pktStatus, void *inBuf,
                                        uint16_t inLenBytes, void *outBuf, uint16_t outLenBytes)
{
    (void)index;
    (void)pktStatus;
    (void)inBuf;
    (void)outBuf;
    (void)outLenBytes;
    return inLenBytes;
}
#endif /* ENABLE_LC3_DECODING */


/* [] END OF FILE */

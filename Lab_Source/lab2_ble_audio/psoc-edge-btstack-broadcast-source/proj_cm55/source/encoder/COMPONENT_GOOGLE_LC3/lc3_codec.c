/******************************************************************************
* File Name        : lc3_codec.c
*
* Description      : This source file contains the implementation for the LC3
*                    codec wrapper APIs using the Google LC3 codec.
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "lc3.h"
#include "lc3_codec.h"
#include "audio_sample.h"

#include "stdio.h"
#include <stdlib.h>

#include "FreeRTOS.h"

#include "wiced_bt_trace.h"
#include "wiced_bt_types.h"

#include "logging.h"

/*******************************************************************************
* Macros
*******************************************************************************/

#ifdef TAG
#undef TAG
#define TAG "[lc3_codec]"
#endif

#define LC3_PLC_OPERATE     (1u)
#define LC3_WRONG_PARAMETER (-1)
#define LC3_SUCCESS         (0)
/* STRIDE is the distance between consecutive samples of one channel.
 * Left and right channels are processed separately, so the stride is 1.
 * Interleaved stereo samples would require a stride of 2.
 */
#define STRIDE              (1u)

/*******************************************************************************
* Global Variables
*******************************************************************************/
lc3_encoder_t enc[ISO_AUDIO_MAX_PARAM_COUNT];
lc3_decoder_t dec[ISO_AUDIO_MAX_PARAM_COUNT];

lc3_config_t enc_audio_cfg[ISO_AUDIO_MAX_PARAM_COUNT];
lc3_config_t dec_audio_cfg[ISO_AUDIO_MAX_PARAM_COUNT];

/*******************************************************************************
*
* Function Name: lc3_codec_reset
*
* Summary: Release encoder and decoder resources at indices with an encoder.
*
* Parameters: None
*
*  Return: None
*
*********************************************************************************/
void lc3_codec_reset(void)
{
    uint8_t index;

    for (index = 0; index < ISO_AUDIO_MAX_PARAM_COUNT; index++)
    {
        if (enc[index] != NULL)
        {
            lc3_codec_releaseEncoder(index);
            lc3_codec_releaseDecoder(index);
        }
    }
}

/*******************************************************************************
*
* Function Name: lc3_codec_initializeDecoder
*
* Summary: Initialize a decoder using the Google LC3 lc3_setup_decoder API.
*
* Parameters:
*   uint8_t index:
*       the index of decoder
*
*   lc3_config_t *p_lc3Config:
*       the decoder audio config
*
*  Return:
*   wiced_bool_t - TRUE if decoder setup succeeds; FALSE otherwise.
*
*********************************************************************************/
wiced_bool_t lc3_codec_initializeDecoder(uint8_t index, lc3_config_t *p_lc3Config)
{
    TRACE_LOG("frame_us:%d, srate_hz:%d, octestPerFrame:%d, samepleWidthInBits:%d\n", p_lc3Config->sduInterval, p_lc3Config->sampleRate, p_lc3Config->octetsPerFrame, p_lc3Config->sampleWidthInBits);
    if (dec[index] != NULL)
    {
        TRACE_LOG("dec at index:%d not NULL, release it\n", index);
        lc3_codec_releaseDecoder(index);
    }
    dec[index] = lc3_setup_decoder(p_lc3Config->sduInterval, p_lc3Config->sampleRate, p_lc3Config->sampleRate,
                                   pvPortMalloc(lc3_decoder_size(p_lc3Config->sduInterval, p_lc3Config->sampleRate)));

    memcpy(&dec_audio_cfg[index], p_lc3Config, sizeof(lc3_config_t));

    return dec[index] == NULL ? FALSE : TRUE;
}

/*******************************************************************************
*
* Function Name: lc3_codec_releaseDecoder
*
* Summary: Release the decoder and clear its audio configuration.
*
* Parameters:
*   uint8_t index:
*       the decoder index
*
*  Return: None
*
*********************************************************************************/
void lc3_codec_releaseDecoder(uint8_t index)
{
    TRACE_LOG("index:%d\n", index);
    if (dec[index] != NULL)
    {
        free(dec[index]);
        dec[index] = NULL;
    }
    else
    {
        TRACE_ERR("dec[%d] is NULL\n", index);
    }
    memset(&dec_audio_cfg[index], 0, sizeof(lc3_config_t));

}

/*******************************************************************************
*
* Function Name: lc3_codec_Decode
*
* Summary: Decode an LC3 frame using the Google LC3 lc3_decode API.
*
* Parameters:
*   uint8_t index:
*       the decoder index
*
*   uint8_t pktStatus:
*       unused packet status
*
*   void *inBuf:
*       the input lc3 data buffer
*
*   uint16_t inLenBytes
*       the input lc3 frame size in bytes
*
*   void *outBuf
*       the pcm output data buffer
*
*   uint16_t outLenBytes
*       output buffer size in bytes, used to clear the buffer before decoding
*
* Return: The supplied outLenBytes, including when decoding reports an error.
*
*********************************************************************************/
uint32_t lc3_codec_Decode(uint8_t index, uint8_t pktStatus, void *inBuf, uint16_t inLenBytes, void *outBuf, uint16_t outLenBytes)
{
    int res = 0;
    uint8_t pcm_sbytes = dec_audio_cfg[index].sampleWidthInBits / 8;
    enum lc3_pcm_format pcm_fmt =
        pcm_sbytes == 32/8 ? LC3_PCM_FORMAT_S24 :
        pcm_sbytes == 24/8 ? LC3_PCM_FORMAT_S24_3LE : LC3_PCM_FORMAT_S16;

    memset(outBuf, 0, outLenBytes);

    res = lc3_decode(dec[index], inBuf, inLenBytes, pcm_fmt, outBuf, STRIDE);

    if (res == LC3_PLC_OPERATE)
    {
        TRACE_LOG("PLC Operated\n");
    }
    if (res == LC3_WRONG_PARAMETER)
    {
        TRACE_ERR("Wrong parameter\n");
    }

    return outLenBytes;
}

/*******************************************************************************
*
* Function Name: lc3_codec_initializeEncoder
*
* Summary: Initialize an encoder using the Google LC3 lc3_setup_encoder API.
*
* Parameters:
*   uint8_t index:
*       the encoder index
*
*   lc3_config_t *p_lc3Config:
*       the audio config
*
*  Return:
*       wiced_bool_t - TRUE if encoder setup succeeds; FALSE otherwise.
*
*********************************************************************************/
wiced_bool_t lc3_codec_initializeEncoder(uint8_t index, lc3_config_t *p_lc3Config)
{
    TRACE_LOG("frame_us:%d, srate_hz:%d, octestPerFrame:%d, samepleWidthInBits:%d\n", p_lc3Config->sduInterval, p_lc3Config->sampleRate, p_lc3Config->octetsPerFrame, p_lc3Config->sampleWidthInBits);
    if (enc[index] != NULL)
    {
        TRACE_LOG("enc at index:%d not NULL, release it\n", index);
        lc3_codec_releaseEncoder(index);
    }
    enc[index] = lc3_setup_encoder(p_lc3Config->sduInterval, p_lc3Config->sampleRate, AUDIO_SAMPLE_RATE_HZ,
                                   pvPortMalloc(lc3_encoder_size(p_lc3Config->sduInterval, AUDIO_SAMPLE_RATE_HZ)));

    memcpy(&enc_audio_cfg[index], p_lc3Config, sizeof(lc3_config_t));

    return enc[index] == NULL ? FALSE : TRUE;
}

/*******************************************************************************
*
* Function Name: lc3_codec_releaseEncoder
*
* Summary: Release the encoder and clear its audio configuration.
*
* Parameters:
*   uint8_t index:
*       the encoder index
*
*  Return: None
*
*********************************************************************************/
void lc3_codec_releaseEncoder(uint8_t index)
{
    TRACE_LOG("index:%d\n", index);
    if (enc[index] != NULL)
    {
        free(enc[index]);
        enc[index] = NULL;
    }
    else
    {
        TRACE_ERR("enc[%d] is NULL\n", index);
    }
    memset(&enc_audio_cfg[index], 0, sizeof(lc3_config_t));
}

/*******************************************************************************
*
* Function Name: lc3_codec_Encode
*
* Summary: Encode PCM samples using the Google LC3 lc3_encode API.
*
* Parameters:
*   uint8_t index:
*       the encoder index
*
*   void *inBuf:
*       the input pcm data buffer
*
*   uint16_t inLenBytes:
*       input buffer size in bytes (unused)
*
*   void *outBuf:
*       the lc3 data output buffer
*
*   uint16_t outLenBytes:
*       LC3 frame size in bytes, passed to the Google LC3 encoder
*
* Return: The supplied outLenBytes, including when encoding reports an error.
*
*********************************************************************************/
uint32_t lc3_codec_Encode(uint8_t index, void *inBuf, uint16_t inLenBytes, void *outBuf, uint16_t outLenBytes)
{
    uint8_t pcm_sbytes = enc_audio_cfg[index].sampleWidthInBits / 8;
    enum lc3_pcm_format pcm_fmt =
        pcm_sbytes == 32/8 ? LC3_PCM_FORMAT_S24 :
        pcm_sbytes == 24/8 ? LC3_PCM_FORMAT_S24_3LE : LC3_PCM_FORMAT_S16;

    memset(outBuf, 0, outLenBytes);

    if (lc3_encode(enc[index], pcm_fmt, inBuf, STRIDE, outLenBytes, outBuf) != LC3_SUCCESS)
    {
        TRACE_ERR("lc3_encode fail\n");
    }

    return outLenBytes;
}

/* [] END OF FILE */

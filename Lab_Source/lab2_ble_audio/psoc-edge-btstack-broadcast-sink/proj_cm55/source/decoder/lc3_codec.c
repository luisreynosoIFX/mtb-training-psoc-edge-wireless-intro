/******************************************************************************
* File Name        : lc3_codec.c
*
* Description      : This file provides the interface for LC3 decoder functions.
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

#include "lc3_codec.h"

#if ENABLE_LC3_DECODING

/******************************************************************************
* Header Files
*******************************************************************************/
#include "ia_apicmd_standards.h"
#include "ia_lc3_dec_config_params.h"
#include "stdio.h"
#include <stdlib.h>
#include "wiced_bt_trace.h"
#include "wiced_bt_types.h"
#include "logging.h"
#include "FreeRTOS.h"


/******************************************************************************
* Macros
*******************************************************************************/
#ifdef TAG
#undef TAG
#define TAG "[lc3_codec]"
#endif

/* Macros for NULL checking */
#define CHECK_FOR_NULL_AND_RETURN_VALUE(x, error_return_value)      \
    if (!x)                                                         \
    {                                                               \
        WICED_BT_TRACE_CRIT("[%s] %s is NULL\n", __FUNCTION__, #x); \
        return error_return_value;                                  \
    }

#define CHECK_FOR_NULL_AND_RETURN(x)                                \
    if (!x)                                                         \
    {                                                               \
        WICED_BT_TRACE_CRIT("[%s] %s is NULL\n", __FUNCTION__, #x); \
        return;                                                     \
    }

#define IA_NO_ERROR 0x00000000
#define IA_DECODE 1
#define IA_ENCODE 2
#define IA_MEMTYPE_INPUT 0x02
#define IA_MEMTYPE_OUTPUT 0x03
#define LC3_EV_DO_WORK 0x01

#define IA_LC3_DEBUG 1

#if IA_LC3_DEBUG > 0
#include "stdio.h"
#endif


/******************************************************************************
* Data structures and enumeration
*******************************************************************************/
typedef enum
{
    IA_MEM_API,
    IA_MEM_TABLES,
    IA_MEM_TABLE1,
    IA_MEM_TABLE2,
    IA_MEM_TABLE3,
    IA_MEM_TABLE4,
    IA_MEM_LAST
} IA_MEMORY_TYPES_t;

typedef enum
{
    IA_CODEC_IDLE,
    IA_CODEC_INITIALIZED,
} IA_DECODER_STATES_t;

typedef struct
{
    void *allocatedMemory[IA_MEM_LAST];
    void *pInputBuf;
    void *pOutputBuf;
    uint16_t inputBufSize;
    uint16_t outputBufSize;
    uint16_t pcmWordSize;
    uint16_t codecState;
} IA_CODEC_CONTEXT_t;


/******************************************************************************
* Global Variables
*******************************************************************************/
IA_CODEC_CONTEXT_t *ialc3_decoderContext[ISO_AUDIO_MAX_PARAM_COUNT] = {NULL, NULL};

#if IA_LC3_DEBUG > 0
char ialc3_errBuf[64];
#endif

/******************************************************************************
* Function Prototypes
*******************************************************************************/
uint32_t ia_lc3_dec_api(void *p_ia_module_obj, uint32_t i_cmd, uint32_t i_idx,
                        void *pv_value);


/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: ia_lc3_allocate_memory
*******************************************************************************
* Summary:
*   Function to allocate memory for the LC3 operations.
*
* Parameters:
*   size : Size of the required buffer in bytes
*   desc : Description for the required buffer
*
* Return:
*   uint32_t* - Pointer output from the dynamically allocated buffer
*
*******************************************************************************/
static uint32_t* ia_lc3_allocate_memory(uint32_t size, const char *desc)
{
    uint32_t *p = NULL;

    WICED_BT_TRACE("[%s] Allocating %lu bytes\n", desc,
                   (unsigned long int) size);
    p = (uint32_t *) pvPortMalloc(size);
    CHECK_FOR_NULL_AND_RETURN_VALUE(p, NULL);
    memset(p, 0, size);

    return p;
}

/******************************************************************************
* Function Name: ia_lc3_free_memory
*******************************************************************************
* Summary:
*   Function to free the allocated memory.
*
* Parameters:
*   memPtr : Pointer to the allocated buffer that needs to be freed
*
* Return:
*   None
*
*******************************************************************************/
static void ia_lc3_free_memory(uint32_t *memPtr)
{
    WICED_BT_TRACE("freeing 0x%p\n", memPtr);
    free(memPtr);
}

/******************************************************************************
* Function Name: ialc3_clearDecoderContext
*******************************************************************************
* Summary:
*   Function to free the allocated decoder resources.
*
* Parameters:
*   index       : Index of the decoder to be cleared.
*   freeContext : If true, the decoder context memory would be freed.
*
* Return:
*   None
*
*******************************************************************************/
static void ialc3_clearDecoderContext(uint8_t index, wiced_bool_t freeContext)
{
    if (ialc3_decoderContext[index] != NULL)
    {
        uint8_t i;

        /* Free any allocated memory */
        for (i = 0; i < IA_MEM_LAST; i++)
        {
            if (NULL != ialc3_decoderContext[index]->allocatedMemory[i])
            {
                ia_lc3_free_memory(ialc3_decoderContext[index]->allocatedMemory[i]);
                ialc3_decoderContext[index]->allocatedMemory[i] = NULL;
            }
        }
        ialc3_decoderContext[index]->pInputBuf = NULL;
        ialc3_decoderContext[index]->pOutputBuf = NULL;
        ialc3_decoderContext[index]->inputBufSize = 0;
        ialc3_decoderContext[index]->outputBufSize = 0;
        ialc3_decoderContext[index]->codecState = IA_CODEC_IDLE;
        if (freeContext)
        {
            ia_lc3_free_memory((uint32_t *)(ialc3_decoderContext[index]));
            ialc3_decoderContext[index] = NULL;
        }
    }
}

/******************************************************************************
* Function Name: lc3_codec_reset
*******************************************************************************
* Summary:
*   Function that clears the context of all decoder instances.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void lc3_codec_reset(void)
{
    uint8_t index;

    for (index = 0; index < ISO_AUDIO_MAX_PARAM_COUNT; index++)
    {
        ialc3_clearDecoderContext(index, TRUE);
    }
}

/******************************************************************************
* Function Name: ialc3_checkError
*******************************************************************************
* Summary:
*   Function to check for errors and print the error string.
*
* Parameters:
*   codec : Indicates if it is an encoder or decoder
*   code  : Error code
*
* Return:
*   wiced_bool_t - TRUE on error, FALSE on no error
*
*******************************************************************************/
static wiced_bool_t ialc3_checkError(uint8_t codec, uint32_t code)
{
#if IA_LC3_DEBUG > 0
    uint32_t isFatal, errClass, errSubCode;
    uint8_t charIndex = 0;
#endif /* IA_LC3_DEBUG */

    if (code == IA_NO_ERROR)
    {
        return FALSE;
    }
#if IA_LC3_DEBUG > 0
    isFatal = (((uint32_t)code & 0x8000) >> 15);
    errClass = (((uint32_t)code & 0x7800) >> 11);
    errSubCode = (((uint32_t)code & 0x07FF));

    if (!isFatal)
    {
        charIndex = sprintf(&ialc3_errBuf[charIndex], "Non-");
    }
    if (codec == IA_DECODE)
    {
        charIndex = sprintf(&ialc3_errBuf[charIndex], "Fatal error: Ittiam lc3_dec ");
    }
    else
    {
        charIndex = sprintf(&ialc3_errBuf[charIndex], "Fatal error: Ittiam lc3_enc ");
    }
    switch (errClass)
    {
    case 0:
        sprintf(&ialc3_errBuf[charIndex], "API code:%lu\n", (unsigned long) errSubCode);
        break;
    case 1:
        sprintf(&ialc3_errBuf[charIndex], "Configuration code:%lu\n", (unsigned long) errSubCode);
        break;
    case 2:
        sprintf(&ialc3_errBuf[charIndex], "Initialization code:%lu\n", (unsigned long) errSubCode);
        break;
    case 3:
        sprintf(&ialc3_errBuf[charIndex], "Execution code:%lu\n", (unsigned long) errSubCode);
        break;
    default:
        sprintf(&ialc3_errBuf[charIndex], "class:%lu code:%lu\n", (unsigned long) errClass,
                (unsigned long) errSubCode);
        break;
    }
    printf("%s\n", ialc3_errBuf);
#endif /* IA_LC3_DEBUG */

    return TRUE;
}

/******************************************************************************
* Function Name: lc3_codec_initializeDecoder
*******************************************************************************
* Summary:
*   Function to allocate all resources and initialize the decoder instance
*
* Parameters:
*   codec_index  : Index of the codec to be initialized
*   p_lc3Config  : Codec configuration
*
* Return:
*   wiced_bool_t - TRUE on initialization success, FALSE on failure
*
*******************************************************************************/
wiced_bool_t lc3_codec_initializeDecoder(uint8_t codec_index, lc3_config_t *p_lc3Config)
{
    uint32_t errCode;
    uint32_t memSize, param, i;
    void *p_iaProcessApiObj = NULL;
    IA_CODEC_CONTEXT_t *context = NULL;

    /* If needed allocate a context block */
    if (ialc3_decoderContext[codec_index] == NULL)
    {
        ialc3_decoderContext[codec_index] = (IA_CODEC_CONTEXT_t *)
            ia_lc3_allocate_memory(sizeof(IA_CODEC_CONTEXT_t), __FUNCTION__);
        if (ialc3_decoderContext[codec_index] == NULL)
        {
            return FALSE;
        }
        ialc3_clearDecoderContext(codec_index, FALSE);
    }

    /* Set the current context */
    context = ialc3_decoderContext[codec_index];

    /* Don't re-initialize or double allocate memory */
    if (context->codecState != IA_CODEC_IDLE)
    {
        TRACE_ERR("codec IA_CODEC_INITIALIZED\n");
        return TRUE;
    }

    /* Get the API size */
    errCode = ia_lc3_dec_api(NULL, IA_API_CMD_GET_API_SIZE, 0, &memSize);

    /* Allocate the API memory */
    if (FALSE == ialc3_checkError(IA_DECODE, errCode))
    {
        context->allocatedMemory[IA_MEM_API] =
                               ia_lc3_allocate_memory(memSize, __FUNCTION__);
        if (NULL == context->allocatedMemory[IA_MEM_API])
        {
            goto cleanUp;
        }
        p_iaProcessApiObj = context->allocatedMemory[IA_MEM_API];
    }
    else
    {
        goto cleanUp;
    }

    /* Default the codec parameters */
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_INIT,
                             IA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
                             NULL);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }

    /* Set the sample width */
    context->pcmWordSize = p_lc3Config->sampleWidthInBits;

    /* Set the codec parameters */
    param = p_lc3Config->sampleWidthInBits;
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_CONFIG_PARAM,
                             IA_LC3_DEC_CONFIG_PARAM_PCM_WDSZ, &param);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    param = p_lc3Config->sampleRate;
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_CONFIG_PARAM,
                             IA_LC3_DEC_CONFIG_PARAM_SAMP_FREQ, &param);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    param = 1;
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_CONFIG_PARAM,
                             IA_LC3_DEC_CONFIG_PARAM_NUM_CHANNELS, &param);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    param = 800 * p_lc3Config->octetsPerFrame;
    if (p_lc3Config->sduInterval == 7500)
    {
        param = (4 * param) / 3;
    }
    if (p_lc3Config->sampleRate == 44100)
    {
        param = (147 * param) / 160;
    }
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_CONFIG_PARAM,
                             IA_LC3_DEC_CONFIG_PARAM_BITRATE, &param);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    param = p_lc3Config->sduInterval / 100;
    if (p_lc3Config->sampleRate == 44100)
    {
        param = (25 * param) / 27;
    }
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_CONFIG_PARAM,
                             IA_LC3_DEC_CONFIG_PARAM_FRAME_MS, &param);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    /* get info table memory size */
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_GET_MEMTABS_SIZE, 0, &memSize);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    context->allocatedMemory[IA_MEM_TABLES] = ia_lc3_allocate_memory(memSize, __FUNCTION__);
    if (NULL == context->allocatedMemory[IA_MEM_TABLES])
    {
        goto cleanUp;
    }
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_MEMTABS_PTR,
                             0, context->allocatedMemory[IA_MEM_TABLES]);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }

    /* Register all the parameters */
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_INIT,
                             IA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS, NULL);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }

    /* Get number of memory tables required */
    errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_GET_N_MEMTABS, 0, &param);
    if (TRUE == ialc3_checkError(IA_DECODE, errCode))
    {
        goto cleanUp;
    }
    for (i = 0; i < param; i++)
    {
        uint32_t alignment, type;
        uint32_t *p_mem;

        /* Get memory size, alignment and type */
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_GET_MEM_INFO_SIZE, i, &memSize);
        errCode |= ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_GET_MEM_INFO_ALIGNMENT, i, &alignment);
        errCode |= ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_GET_MEM_INFO_TYPE, i, &type);
        if (IA_NO_ERROR != errCode)
        {
            goto cleanUp;
        }
        if (alignment > 4)
        {
            memSize += alignment - 4;
        }
        context->allocatedMemory[IA_MEM_TABLE1 + i] = ia_lc3_allocate_memory(memSize, __FUNCTION__);
        if (NULL == context->allocatedMemory[IA_MEM_TABLE1 + i])
        {
            goto cleanUp;
        }
        p_mem = context->allocatedMemory[IA_MEM_TABLE1 + i];
        if (alignment > 4)
        {
            p_mem = (uint32_t *)(((uintptr_t)p_mem + (alignment - 4)) & ~(uintptr_t)(alignment - 1));
        }
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_MEM_PTR, i, p_mem);
        if (TRUE == ialc3_checkError(IA_DECODE, errCode))
        {
            goto cleanUp;
        }
        if (type == IA_MEMTYPE_INPUT)
        {
            context->pInputBuf = p_mem;
            context->inputBufSize = memSize;
        }
        if (type == IA_MEMTYPE_OUTPUT)
        {
            context->pOutputBuf = p_mem;
            context->outputBufSize = memSize;
        }
    }

    /* Initialize process */
    do
    {
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_INIT, IA_CMD_TYPE_INIT_PROCESS, NULL);
        errCode |= ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_INIT, IA_CMD_TYPE_INIT_DONE_QUERY, &param);
        if (errCode != IA_NO_ERROR)
        {
            goto cleanUp;
        }
    } while (!param);
    context->codecState = IA_CODEC_INITIALIZED;
    return TRUE;

cleanUp:
    ialc3_clearDecoderContext(codec_index, TRUE);
    return FALSE;
}

/******************************************************************************
* Function Name: lc3_codec_releaseDecoder
*******************************************************************************
* Summary:
*   Function that clears the context of the decoder for a given instance.
*
* Parameters:
*   index : Index of the decoder to be cleared
*
* Return:
*   None
*
*******************************************************************************/
void lc3_codec_releaseDecoder(uint8_t index)
{
    ialc3_clearDecoderContext(index, TRUE);
}

/******************************************************************************
* Function Name: ia_lc3_getDecoder
*******************************************************************************
* Summary:
*   Function to extract the output PCM from decoder output based on the PCM
*   word size.
*
* Parameters:
*   pb_out_buf : Pointer to the 32-bit integer buffer (Input)
*   p_pcm_out  : Pointer to the final PCM output data (Output)
*   num_bytes  : Number of bytes of data to be processed
*   pcm_wd_sz  : Bit-width of the PCM data (16-bit and 24-bit supported)
*
* Return:
*   None
*
*******************************************************************************/
static void ia_lc3_getDecoder(int32_t *pb_out_buf, void *p_pcm_out,
                              uint16_t num_bytes, uint16_t pcm_wd_sz)
{
    uint32_t i, num_samples;

    /* 16-bit sample size */
    if (pcm_wd_sz == 16)
    {
        int16_t *p_pcm_s16le;

        p_pcm_s16le = (int16_t *)p_pcm_out;
        num_samples = num_bytes >> 1;
        for (i = 0; i < num_samples; i++)
        {
            p_pcm_s16le[i] = (int16_t)(pb_out_buf[i]);
        }
    }
    else /* 24-bit sample size */
    {
        int8_t *p_pcm_s24le;

        /* This assumes that 24-bit samples should be byte packed */
        p_pcm_s24le = (int8_t *)p_pcm_out;
        num_samples = num_bytes / 3;
        for (i = 0; i < num_samples; i++)
        {
            *p_pcm_s24le++ = (int8_t)((pb_out_buf[i] << 24) >> 24);
            *p_pcm_s24le++ = (int8_t)((pb_out_buf[i] << 16) >> 24);
            *p_pcm_s24le++ = (int8_t)((pb_out_buf[i] << 8) >> 24);
        }
    }
}

/******************************************************************************
* Function Name: lc3_codec_Decode
*******************************************************************************
* Summary:
*   Function to decode an LC3 packet into PCM samples.
*
* Parameters:
*   index       : Index of the LC3 decoder to be used
*   pktStatus   : LC3 packet status value (not used)
*   inBuf       : Pointer to the buffer containing input LC3 data (Input)
*   inLenBytes  : Size of the above input buffer in bytes
*   outBuf      : Pointer to the buffer for output PCM data (Output)
*   outLenBytes : Size of the above output buffer in bytes
*
* Return:
*   uint32_t - Output buffer size in bytes
*
*******************************************************************************/
uint32_t lc3_codec_Decode(uint8_t index, uint8_t pktStatus, void *inBuf,
                          uint16_t inLenBytes, void *outBuf, uint16_t outLenBytes)
{
    IA_CODEC_CONTEXT_t *context = NULL;
    void *p_iaProcessApiObj = NULL;
    uint32_t param;
    uint32_t errCode;

    /* Fill the output buffer with silence */
    memset(outBuf, 0, outLenBytes);

    /* Get the decoder context */
    context = ialc3_decoderContext[index];
    if ((context == NULL) || (context->codecState != IA_CODEC_INITIALIZED))
    {
        ialc3_checkError(IA_DECODE, 0xE8);
        return outLenBytes;
    }

    /* Point to the API memory */
    p_iaProcessApiObj = context->allocatedMemory[IA_MEM_API];

    /* Check the input buffer size against what was allocated */
    if ((4 * inLenBytes) <= context->inputBufSize)
    {
        /* Move the packet to the input buffer */
        memcpy(context->pInputBuf, inBuf, inLenBytes);

        /* Set the input buffer size */
        param = inLenBytes;
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_INPUT_BYTES, 0, &param);
        if (TRUE == ialc3_checkError(IA_DECODE, errCode))
        {
            return outLenBytes;
        }
        param = 0;
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_SET_BFI_EXT, 0, &param);
        if (TRUE == ialc3_checkError(IA_DECODE, errCode))
        {
            return outLenBytes;
        }

        /* Process the buffer */
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_EXECUTE, IA_CMD_TYPE_DO_EXECUTE, NULL);
        if (TRUE == ialc3_checkError(IA_DECODE, errCode))
        {
            return outLenBytes;
        }

        /* Get bytes processed */
        errCode = ia_lc3_dec_api(p_iaProcessApiObj, IA_API_CMD_GET_OUTPUT_BYTES, 0, &param);
        if (TRUE == ialc3_checkError(IA_DECODE, errCode))
        {
            return outLenBytes;
        }
        if (param != outLenBytes)
        {
            ialc3_checkError(IA_DECODE, 0xE9);
            WICED_BT_TRACE("[param : %lu] [outLenBytes : %d]\n",
                           (unsigned long int) param, outLenBytes);
            return outLenBytes;
        }

        /* Decode takes so long that the context may have been deleted while it was running */
        if ((NULL != ialc3_decoderContext[index]) && (NULL != context->pOutputBuf))
        {
            ia_lc3_getDecoder(context->pOutputBuf, outBuf, outLenBytes, context->pcmWordSize);
        }
    }
    return outLenBytes;
}

#endif /* ENABLE_LC3_DECODING */

/* [] END OF FILE */

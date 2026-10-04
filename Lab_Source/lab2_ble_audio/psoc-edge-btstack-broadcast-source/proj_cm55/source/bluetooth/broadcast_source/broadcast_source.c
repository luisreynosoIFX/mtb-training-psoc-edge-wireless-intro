/******************************************************************************
* File Name        : broadcast_source.c
*
* Description      : Broadcast Source application logic for PSoC Edge.
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
#include "broadcast_source.h"
#include <string.h>
#include "broadcast_source_bis.h"
#include "broadcast_source_iso_audio.h"
#include "app_bt_utils.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_ga_bap.h"
#include "wiced_bt_trace.h"
#include "logging.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define DEFAULT_BIS_START             0
#define DEFAULT_BIS_CODEC             BAP_CODEC_CONFIG_48_2_2
#define DEFAULT_BIS_ENCRYPTION        0
#ifdef AUDIO_CONFIG_MONO
#define DEFAULT_BIS_CHANNEL_COUNT     1
#else
#define DEFAULT_BIS_CHANNEL_COUNT     2
#endif
#define DEFAULT_BIS_BROADCAST_ID     16
#define DEFAULT_BIS_COUNT             1

/******************************************************************************
* Extern Variables
*******************************************************************************/
extern wiced_bt_cfg_isoc_t broadcast_source_isoc_cfg;

/******************************************************************************
* Global Variables
*******************************************************************************/
static char *codec_config_array[] =
{
    "16_2 (10ms-32kpbs)",
    "48_2 (10ms-80kpbs)",
    "48_4 (10ms-96kpbs)",
    "48_6 (10ms-124kpbs)",
    "32_2 (10ms-64kpbs)",
    "24_2 (10ms-48kbps)",
    "8_2 (10ms-24kbps)",
};

static broadcast_stream_config g_bis_config;
static wiced_bool_t g_broadcast_started = FALSE;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void set_default_broadcast_stream_config(broadcast_stream_config *config);
static char *codec_config_mapping(uint32_t input);
static void print_broadcast_code(uint8_t *code, int size);

/******************************************************************************
* Function Definitions
*******************************************************************************/
/*******************************************************************************
* Function Name: broadcast_source_init_and_start
********************************************************************************
* Summary:
*   Initialize BIS support and start broadcasting the default configuration once.
*
* Parameters:
*   None
*
* Return:
*   None
*******************************************************************************/
void broadcast_source_init_and_start(void)
{
    if (g_broadcast_started)
    {
        return;
    }
    g_broadcast_started = TRUE;

    broadcast_source_bis_init(&broadcast_source_isoc_cfg);

    set_default_broadcast_stream_config(&g_bis_config);
    g_bis_config.start = 1;

    print_broadcast_stream_config(g_bis_config);
    broadcast_source_handle_start_streaming(g_bis_config);
}

/*******************************************************************************
* Function Name: set_default_broadcast_stream_config
********************************************************************************
* Summary:
*   Assign default broadcast_stream_config settings.
*
* Parameters:
*   broadcast_stream_config *config
*
* Return:
*   None
*
*******************************************************************************/
static void set_default_broadcast_stream_config(broadcast_stream_config *config)
{
    config->start = DEFAULT_BIS_START;
    config->codec_config = DEFAULT_BIS_CODEC;
    config->enable_encryption = DEFAULT_BIS_ENCRYPTION;
    config->channel_counts = DEFAULT_BIS_CHANNEL_COUNT;
    config->broadcast_id = DEFAULT_BIS_BROADCAST_ID;
    config->bis_count = DEFAULT_BIS_COUNT;
    memset(config->broadcast_code, '\0', MAX_BROADCASTCODE_LEN);
}

/*******************************************************************************
* Function Name: codec_config_mapping
********************************************************************************
* Summary:
*   Get string from codec number (wiced_bt_ga_bap_codec_config_t)
*
* Parameters:
*   uint32_t input
*
* Return:
*   char *
*
*******************************************************************************/
static char *codec_config_mapping(uint32_t input)
{
    switch (input)
    {
        case BAP_CODEC_CONFIG_16_2_2:
            return codec_config_array[0];
        case BAP_CODEC_CONFIG_48_2_2:
            return codec_config_array[1];
        case BAP_CODEC_CONFIG_48_4_2:
            return codec_config_array[2];
        case BAP_CODEC_CONFIG_48_6_2:
            return codec_config_array[3];
        case BAP_CODEC_CONFIG_32_2_2:
            return codec_config_array[4];
        case BAP_CODEC_CONFIG_24_2_2:
            return codec_config_array[5];
        case BAP_CODEC_CONFIG_8_2_2:
            return codec_config_array[6];
        default:
            return "Not Setting";
    }
}

/*******************************************************************************
* Function Name: print_broadcast_code
********************************************************************************
* Summary:
*   Print broadcast code as hex string.
*
* Parameters:
*   uint8_t *code
*   int size
*
* Return:
*   None
*
*******************************************************************************/
static void print_broadcast_code(uint8_t *code, int size)
{
    WICED_BT_TRACE("broadcast_code:");
    for (int i = 0; i < size; i++)
    {
        WICED_BT_TRACE("%02X", code[i]);
    }
    WICED_BT_TRACE("\n\n");
}

/*******************************************************************************
* Function Name: print_broadcast_stream_config
********************************************************************************
* Summary:
*   Print the settings of broadcast_stream_config.
*
* Parameters:
*   broadcast_stream_config config
*
* Return:
*   None
*
*******************************************************************************/
void print_broadcast_stream_config(broadcast_stream_config config)
{
    uint8_t start = config.start;
    uint32_t codec_config = config.codec_config;
    uint8_t enable_encryption = config.enable_encryption;
    uint32_t channel_counts = config.channel_counts;
    uint32_t broadcast_id = config.broadcast_id;
    uint8_t broadcast_code[MAX_BROADCASTCODE_LEN];

    memcpy(broadcast_code, config.broadcast_code, MAX_BROADCASTCODE_LEN);

    TRACE_LOG("Start:%d, codec_config:%s, enable_encryption:%d channel_counts:%lu Broadcast ID:0x%lx ",
              start,
              codec_config_mapping(codec_config),
              enable_encryption,
              (unsigned long)channel_counts,
              (unsigned long)broadcast_id);
    print_broadcast_code(broadcast_code, MAX_BROADCASTCODE_LEN);
}

/*******************************************************************************
* Function Name: broadcast_source_handle_start_streaming
********************************************************************************
* Summary:
*   Broadcast Source start streaming.
*
* Parameters:
*   broadcast_stream_config config
*
* Return:
*   None
*
*******************************************************************************/
void broadcast_source_handle_start_streaming(broadcast_stream_config config)
{
    wiced_result_t ret_sts = WICED_ERROR;
    wiced_bt_ga_bap_stream_config_t stream_config;
    uint8_t start = config.start;
    uint32_t codec_config = config.codec_config;
    uint8_t enable_encryption = config.enable_encryption;
    uint32_t channel_counts = config.channel_counts;
    uint32_t broadcast_id = config.broadcast_id;
    uint8_t broadcast_code[MAX_BROADCASTCODE_LEN];
    uint8_t bis_count = config.bis_count;

    memcpy(broadcast_code, config.broadcast_code, MAX_BROADCASTCODE_LEN);

    TRACE_LOG("start:%d, codec_config:%s,enable_encryption:%d channel_counts:%lu Broadcast ID:%lu",
              start,
              codec_config_mapping(codec_config),
              enable_encryption,
              (unsigned long)channel_counts,
              (unsigned long)broadcast_id);

    if (start)
    {
        wiced_bt_ga_bap_get_broadcast_stream_config(codec_config, &stream_config);

        TRACE_LOG("Configuring source stream [SF:%lu] [FD:%lu] [OPF:%d] [Latency:%d] [SDU_Interval:%lu] [Framing:%d] [RTN:%d]",
                  (unsigned long)stream_config.sampling_frequency,
                  (unsigned long)stream_config.frame_duration,
                  stream_config.octets_per_codec_frame,
                  stream_config.max_transport_latency,
                  (unsigned long)stream_config.sdu_interval,
                  (int)stream_config.framing,
                  (int)stream_config.retransmission_number);

        ret_sts = broadcast_source_bis_configure_stream(broadcast_id,
                                                        broadcast_code,
                                                        bis_count,
                                                        channel_counts,
                                                        stream_config.sampling_frequency,
                                                        stream_config.frame_duration,
                                                        stream_config.octets_per_codec_frame,
                                                        enable_encryption);
        TRACE_LOG("broadcast_source_bis_configure_stream [ret_sts:0x%x]", ret_sts);

        if (ret_sts != WICED_SUCCESS)
        {
            printf(">> Error: Broadcast configuration failed (0x%02x).\n", ret_sts);
            return;
        }

        broadcast_source_bis_start_stream(&stream_config);
    }
    else
    {
        ret_sts = broadcast_source_bis_disable_stream();
        TRACE_LOG("broadcast_source_bis_disable_stream [ret_sts:0x%x]", ret_sts);

        ret_sts = broadcast_source_bis_release_stream();
        TRACE_LOG("broadcast_source_bis_release_stream [ret_sts:0x%x]", ret_sts);
    }
}

/*******************************************************************************
* Function Name: is_broadcast_streaming
********************************************************************************
* Summary:
*   Check whether the configuration requests streaming; not the live stream state.
*
* Parameters:
*   broadcast_stream_config config
*
* Return:
*   bool - True when config.start equals 1; false otherwise.
*
*******************************************************************************/
bool is_broadcast_streaming(broadcast_stream_config config)
{
    return (config.start == 1) ? true : false;
}

/*******************************************************************************
* Function Name: set_broadcast_stream_config_codec
********************************************************************************
* Summary:
*   Set codec for broadcast_stream_config.
*
* Parameters:
*   uint32_t codec
*   broadcast_stream_config *config
*
* Return:
*   None
*
*******************************************************************************/
void set_broadcast_stream_config_codec(uint32_t codec, broadcast_stream_config *config)
{
    if (config != NULL)
    {
        config->codec_config = codec;
    }
    else
    {
        TRACE_ERR("config is NULL");
    }
}

/*******************************************************************************
* Function Name: set_broadcast_stream_config_encryption
********************************************************************************
* Summary:
*   Set encryption for broadcast_stream_config.
*
* Parameters:
*   bool enable
*   broadcast_stream_config *config
*
* Return:
*   None
*
*******************************************************************************/
void set_broadcast_stream_config_encryption(bool enable, broadcast_stream_config *config)
{
    if (config != NULL)
    {
        config->enable_encryption = (enable == true) ? 1 : 0;
    }
    else
    {
        TRACE_ERR("config is NULL");
    }
}

/*******************************************************************************
* Function Name: set_broadcast_stream_config_channelcounts
********************************************************************************
* Summary:
*   Set channel_counts for broadcast_stream_config.
*
* Parameters:
*   uint32_t channel_counts
*   broadcast_stream_config *config
*
* Return:
*   None
*
*******************************************************************************/
void set_broadcast_stream_config_channelcounts(uint32_t channel_counts, broadcast_stream_config *config)
{
    if (config != NULL)
    {
        config->channel_counts = channel_counts;
    }
    else
    {
        TRACE_ERR("config is NULL");
    }
}

/*******************************************************************************
* Function Name: set_broadcast_stream_config_broadcastid
********************************************************************************
* Summary:
*   Set broadcast id for broadcast_stream_config.
*
* Parameters:
*   uint32_t broadcast_id
*   broadcast_stream_config *config
*
* Return:
*   None
*
*******************************************************************************/
void set_broadcast_stream_config_broadcastid(uint32_t broadcast_id, broadcast_stream_config *config)
{
    if (config != NULL)
    {
        if (broadcast_id == 0 || broadcast_id > MAX_BROADCAST_ID)
        {
            TRACE_ERR("broadcast_id %lx out of range from 1 ~ f423f", (unsigned long)broadcast_id);
        }
        else
        {
            config->broadcast_id = broadcast_id;
        }
    }
    else
    {
        TRACE_ERR("config is NULL");
    }
}

/*******************************************************************************
* Function Name: set_broadcast_stream_config_broadcastcode
********************************************************************************
* Summary:
*   Set broadcast code for broadcast_stream_config.
*
* Parameters:
*   int8_t *broadcast_code
*   broadcast_stream_config *config
*
* Return:
*   None
*
*******************************************************************************/
void set_broadcast_stream_config_broadcastcode(int8_t *broadcast_code, broadcast_stream_config *config)
{
    if (config != NULL)
    {
        memcpy(config->broadcast_code, broadcast_code, MAX_BROADCASTCODE_LEN);
        print_broadcast_code(config->broadcast_code, MAX_BROADCASTCODE_LEN);
    }
    else
    {
        TRACE_ERR("config is NULL");
    }
}

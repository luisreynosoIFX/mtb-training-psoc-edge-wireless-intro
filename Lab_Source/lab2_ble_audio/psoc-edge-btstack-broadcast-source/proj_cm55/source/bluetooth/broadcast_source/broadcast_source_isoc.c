/******************************************************************************
* File Name        : broadcast_source_isoc.c
*
* Description      : Broadcast Source ISO data path and LC3 encoding control.
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
#include <stdint.h>
#include <string.h>

#if ENABLE_LC3_ENCODING
#include "audio_sample.h"
#include "lc3_codec.h"
#else
#include "audio_sample_lc3.h"
#endif
#include "broadcast_source_iso_audio.h"
#include "iso_data_handler.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
#include "logging.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define MAX_INPUT_SAMPLE_SIZE_IN_BYTES  (480 * 2) /* 48kHz @ 10ms, 16-bit */
#define MAX_INPUT_SAMPLES               (MAX_INPUT_SAMPLE_SIZE_IN_BYTES / 2)
#define MAX_STREAMS_SUPPORTED           2
/*******************************************************************************
* Global Variables
*******************************************************************************/
static wiced_bt_pool_t *ga_iso_audio_pool = NULL;
static uint8_t *p_buf = NULL;
static uint32_t s_audio_idx = 0;

/*******************************************************************************
* Data Types
*******************************************************************************/
typedef struct
{
    wiced_bool_t is_cis;
    uint16_t conn_hdl;
    uint16_t octets_per_frame;
    uint8_t num_of_channels;
    wiced_bool_t b_stream_active;
    wiced_bool_t transmission_reported;
    uint32_t frame_duration;
    uint32_t sampling_frequency;
} ga_iso_audio_stream_info_t;

static ga_iso_audio_stream_info_t g_stream_info[MAX_STREAMS_SUPPORTED] = { 0 };

static void audio_source_reset(void)
{
    s_audio_idx = 0;
}

#if ENABLE_LC3_ENCODING
static void fill_from_file(int16_t *dst, uint32_t sample_count)
{
    uint32_t remaining = audio_sample_count - s_audio_idx;
    uint32_t count = remaining < sample_count ? remaining : sample_count;
    memset(dst, 0, sample_count * sizeof(*dst));
    memcpy(dst, audio_sample_data + s_audio_idx, count * sizeof(*dst));
}
#endif

static void init_stream_info(uint16_t conn_hdl,
                             wiced_bool_t is_cis,
                             uint32_t sampling_frequency,
                             uint16_t octets_per_frame,
                             uint32_t frame_duration,
                             uint8_t num_of_channels)
{
    WICED_BT_TRACE("[%s] [is_cis %d] [SF %lu] [OPF %d] [FD %lu] [num_of_channels %d]\n",
        __FUNCTION__,
        is_cis,
        (unsigned long int)sampling_frequency,
        octets_per_frame,
        (unsigned long int)frame_duration,
        num_of_channels);

    for (size_t i = 0; i < MAX_STREAMS_SUPPORTED; i++)
    {
        if (!g_stream_info[i].conn_hdl)
        {
            g_stream_info[i].conn_hdl = conn_hdl;
            g_stream_info[i].is_cis = is_cis;
            g_stream_info[i].octets_per_frame = octets_per_frame;
            g_stream_info[i].num_of_channels = num_of_channels;
            g_stream_info[i].sampling_frequency = sampling_frequency;
            g_stream_info[i].frame_duration = frame_duration;
            g_stream_info[i].b_stream_active = 1;

            WICED_BT_TRACE("[%s] conn_hdl 0x%x \n", __FUNCTION__, conn_hdl);
            return;
        }
    }
}

static ga_iso_audio_stream_info_t *get_stream_info(uint16_t conn_hdl)
{
    for (size_t i = 0; i < MAX_STREAMS_SUPPORTED; i++)
    {
        if (conn_hdl == g_stream_info[i].conn_hdl)
        {
            return &g_stream_info[i];
        }
    }

    return NULL;
}

static void deinit_stream_info(uint16_t conn_hdl)
{
    WICED_BT_TRACE("[%s] [conn_hdl %d]\n", __FUNCTION__, conn_hdl);

    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_hdl);
    if (!p_stream_info) return;

    memset(p_stream_info, 0, sizeof(ga_iso_audio_stream_info_t));
    WICED_BT_TRACE("[%s] deinit successful\n", __FUNCTION__);
}

static uint32_t count_set_bits(uint32_t n)
{
    uint32_t count = 0;
    while (n)
    {
        n &= (n - 1);
        count++;
    }
    return count;
}

/*******************************************************************************
* Function Name: iso_audio_setup_data_path
********************************************************************************
* Summary:
*   Request an HCI data path for an existing BIS and initialize LC3 stream state.
*
* Parameters:
*   conn_hdl  : BIS connection handle.
*   direction : ISO data path direction bit.
*   p_csc     : Codec-specific configuration.
*
* Return:
*   WICED_SUCCESS on setup; an error for invalid or unsupported parameters,
*   a failed data path request, or failed codec initialization.
*******************************************************************************/
wiced_result_t iso_audio_setup_data_path(uint16_t conn_hdl, uint16_t direction, wiced_bt_ga_bap_csc_t *p_csc)
{
#if ENABLE_LC3_ENCODING
    lc3_config_t codec_config;
    wiced_bool_t codec_config_sts;
#endif
    wiced_ble_isoc_setup_data_path_info_t setup_info = {0};
    wiced_ble_isoc_data_path_direction_t data_path_dir = WICED_BLE_ISOC_DPD_MAX_DIRECTIONS;
    int numOfChannels = 1;
    wiced_bool_t is_bis = FALSE;

    WICED_BT_TRACE("[%s] conn_hdl [0x%x] dir [%d] p_csc [0x%x]\n", __FUNCTION__, conn_hdl, direction, (unsigned int)p_csc);
    if (!p_csc) return WICED_BADARG;

    /* check if BIS is created before setting up data path */
    is_bis = wiced_ble_isoc_is_bis_created(conn_hdl);
    if (!is_bis)
    {
        return WICED_BADARG;
    }

    WICED_BT_TRACE("[%s] is_bis %d \n", __FUNCTION__, is_bis);

    if (p_csc->audio_channel_allocation)
    {
        numOfChannels = (int)count_set_bits(p_csc->audio_channel_allocation);
        if (numOfChannels > 2)
        {
            return WICED_UNSUPPORTED;
        }
    }

    data_path_dir = (direction == WICED_BLE_ISOC_DPD_INPUT_BIT) ? WICED_BLE_ISOC_DPD_INPUT : WICED_BLE_ISOC_DPD_OUTPUT;

    WICED_BT_TRACE("[%s] dir %s sampleRate %lu sduInterval %lu octetsPerFrame %d numOfChannels %d conn_hdl 0x%x\n",
        __FUNCTION__,
        (data_path_dir == WICED_BLE_ISOC_DPD_INPUT) ? "Source" : "Sink",
        (unsigned long int)p_csc->sampling_frequency,
        (unsigned long int)p_csc->frame_duration,
        p_csc->octets_per_codec_frame,
        numOfChannels,
        conn_hdl);

    if (!p_csc->sampling_frequency || !p_csc->frame_duration || !p_csc->octets_per_codec_frame) return WICED_ERROR;

#if !ENABLE_LC3_ENCODING
    if (direction != WICED_BLE_ISOC_DPD_INPUT_BIT ||
        !audio_lc3_format_supported(p_csc->sampling_frequency, p_csc->frame_duration,
                                   p_csc->octets_per_codec_frame, (uint8_t)numOfChannels))
    {
        printf(">> Error: Pre-encoded audio requires 48000 Hz, 10 ms, 100 octets/channel, mono or stereo.\n");
        return WICED_UNSUPPORTED;
    }
#endif

    setup_info.isoc_conn_hdl = conn_hdl;
    setup_info.data_path_dir = data_path_dir;
    setup_info.data_path_id = WICED_BLE_ISOC_DPID_HCI;
    setup_info.controller_delay = 0;
    setup_info.csc_length = 0;
    setup_info.p_csc = NULL;
    setup_info.p_app_ctx = NULL;

    if (WICED_SUCCESS != wiced_ble_isoc_setup_data_path(&setup_info))
    {
        WICED_BT_TRACE("[%s] wiced_ble_isoc_setup_data_path failed\n", __FUNCTION__);
        return WICED_ERROR;
    }

#if ENABLE_LC3_ENCODING
    codec_config.sampleRate = (uint16_t)p_csc->sampling_frequency;
    codec_config.sduInterval = (uint16_t)p_csc->frame_duration;
    codec_config.octetsPerFrame = p_csc->octets_per_codec_frame;
    codec_config.sampleWidthInBits = 16;

    for (int i = 0; i < numOfChannels; i++)
    {
        codec_config_sts = (direction == WICED_BLE_ISOC_DPD_INPUT_BIT) ? lc3_codec_initializeEncoder(i, &codec_config)
            : lc3_codec_initializeDecoder(i, &codec_config);
        if (FALSE == codec_config_sts)
        {
            WICED_BT_TRACE_CRIT("[%s] lc3_codec_initialize NOT SUCCESSFUL", __FUNCTION__);
            return WICED_ERROR;
        }
    }

#endif

    init_stream_info(conn_hdl,
        0,
        p_csc->sampling_frequency,
        p_csc->octets_per_codec_frame,
        p_csc->frame_duration,
        (uint8_t)numOfChannels);

    audio_source_reset();

    return WICED_SUCCESS;
}

void iso_audio_remove_data_path(uint16_t conn_hdl, wiced_ble_isoc_data_path_bit_t dir, uint8_t *idx_list)
{
    wiced_bool_t is_bis = FALSE;
    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_hdl);

    (void)idx_list;

    if (p_stream_info == NULL) return;

#if ENABLE_LC3_ENCODING
    int num_channels = p_stream_info->num_of_channels;
#endif

    WICED_BT_TRACE("[%s] [dir %d] [conn_hdl %x] [num of channel %x]",
        __FUNCTION__,
        dir,
        conn_hdl,
        p_stream_info->num_of_channels);

    deinit_stream_info(conn_hdl);

#if ENABLE_LC3_ENCODING
    if (WICED_BLE_ISOC_DPD_INPUT_BIT & dir)
    {
        for (int i = 0; i < num_channels; i++)
        {
            lc3_codec_releaseEncoder(i);
        }
    }
    else if (WICED_BLE_ISOC_DPD_OUTPUT_BIT & dir)
    {
        for (int i = 0; i < num_channels; i++)
        {
            lc3_codec_releaseDecoder(i);
        }
    }

#endif

    is_bis = wiced_ble_isoc_is_bis_created(conn_hdl);
    if (!is_bis)
        return;

    if (!wiced_ble_isoc_remove_data_path(conn_hdl, dir, NULL))
        WICED_BT_TRACE_CRIT("[%s] No active data path\n", __FUNCTION__);
}

static void tx_iso_data(wiced_bool_t is_cis,
                        uint16_t conn_handle,
                        uint16_t octets_per_frame,
                        uint8_t num_of_channels,
                        uint16_t frame_duration)
{
#if ENABLE_LC3_ENCODING
    /* Static to avoid stack overflow in BT task context (1920 bytes per call otherwise) */
    static int16_t pcm_l[MAX_INPUT_SAMPLES];
    static int16_t pcm_r[MAX_INPUT_SAMPLES];
    uint32_t sample_rate = 0;
    uint32_t samples_per_frame = 0;
#else
    (void)frame_duration;
#endif
    uint8_t *p_data = NULL;

    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_handle);

    if (!p_stream_info || !p_buf)
    {
        WICED_BT_TRACE_CRIT("[%s] p_buf or stream info is NULL\n", __FUNCTION__);
        return;
    }

    p_data = p_buf + iso_dhm_get_header_size();

#if ENABLE_LC3_ENCODING
    sample_rate = AUDIO_SAMPLE_RATE_HZ;
    samples_per_frame = (sample_rate * frame_duration) / 1000000;
    if (samples_per_frame > MAX_INPUT_SAMPLES)
    {
        samples_per_frame = MAX_INPUT_SAMPLES;
    }

    fill_from_file(pcm_l, samples_per_frame);
    lc3_codec_Encode(0, pcm_l, (uint16_t)(samples_per_frame * sizeof(int16_t)), p_data, octets_per_frame);

    if (2 == num_of_channels)
    {
        fill_from_file(pcm_r, samples_per_frame);
        lc3_codec_Encode(1,
            pcm_r,
            (uint16_t)(samples_per_frame * sizeof(int16_t)),
            p_data + octets_per_frame,
            octets_per_frame);
    }

    s_audio_idx += samples_per_frame;
    if (s_audio_idx >= audio_sample_count)
        s_audio_idx = 0;  /* loop */
#else
    if (!audio_lc3_copy_frame(&s_audio_idx, num_of_channels, p_data,
                             octets_per_frame * num_of_channels))
        return;
#endif

    iso_dhm_send_packet(is_cis, conn_handle, 0, p_buf, octets_per_frame * num_of_channels);

}

void iso_audio_start_stream(uint16_t conn_hdl)
{
    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_hdl);

    WICED_BT_TRACE("[%s] p_stream_info 0x%x conn_hdl 0x%x\n", __FUNCTION__, p_stream_info, conn_hdl);
    if (!p_stream_info)
    {
        printf(">> Error: Cannot start broadcast audio: stream configuration missing.\n");
        return;
    }

    p_buf = (uint8_t *)wiced_bt_get_buffer_from_pool(ga_iso_audio_pool);
    if (!p_buf)
    {
        printf(">> Error: Cannot start broadcast audio: no transmit buffer.\n");
        return;
    }
    p_stream_info->transmission_reported = FALSE;

    /* Prime the controller pipeline with an initial burst equal to buffer depth.
     * pcm_l/pcm_r are static to avoid stack overflow. Subsequent sends are
     * driven by num_complete_handler (credit-based, matches Linux source). */
    for (int i = 0; i < 4; i++)
    {
        tx_iso_data(p_stream_info->is_cis,
                    conn_hdl,
                    p_stream_info->octets_per_frame,
                    p_stream_info->num_of_channels,
                    (uint16_t)p_stream_info->frame_duration);
    }
}

void iso_audio_stop_stream(uint16_t conn_hdl)
{
    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_hdl);
    if (p_stream_info)
        p_stream_info->b_stream_active = 0;
    if (p_buf)
    {
        wiced_bt_free_buffer(p_buf);
        p_buf = NULL;
    }
}

static void num_complete_handler(uint16_t conn_hdl, uint16_t num_sent)
{
    static uint32_t s_num_complete_count = 0;
    s_num_complete_count++;
    if (s_num_complete_count <= 3 || (s_num_complete_count % 200) == 0)
    {
        WICED_BT_TRACE("[num_complete_handler] #%lu conn_hdl=0x%x num_sent=%d\n",
               (unsigned long)s_num_complete_count, (unsigned int)conn_hdl, (int)num_sent);
    }

    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_hdl);

    if (p_stream_info && p_stream_info->b_stream_active && p_buf &&
        num_sent > 0 && !p_stream_info->transmission_reported)
    {
        p_stream_info->transmission_reported = TRUE;
        printf("Audio broadcasting started. Use a Broadcast Sink to listen.\n");
    }

    for (int i = 0; i < num_sent; i++)
    {
        if (!p_stream_info || !p_stream_info->b_stream_active) return;
        tx_iso_data(p_stream_info->is_cis,
                    conn_hdl,
                    p_stream_info->octets_per_frame,
                    p_stream_info->num_of_channels,
                    (uint16_t)p_stream_info->frame_duration);
    }
}

static void rx_handler(uint16_t conn_hdl, uint8_t *p_data, uint32_t length)
{
    (void)conn_hdl;
    (void)p_data;
    (void)length;
}

static void iso_create_pool(const wiced_bt_cfg_isoc_t *p_iso_cfg)
{
    int buff_size = iso_dhm_get_buffer_size(p_iso_cfg);

    /* Allocate only once, allowing multiple calls to update callbacks */
    if (!ga_iso_audio_pool)
        ga_iso_audio_pool =
            wiced_bt_create_pool("ISO SDU", buff_size, p_iso_cfg->max_buffers_per_cis, NULL);

    if (!ga_iso_audio_pool)
    {
        WICED_BT_TRACE_CRIT("[%s] ga_iso_audio_pool is NULL\n", __FUNCTION__);
        return;
    }

    WICED_BT_TRACE("[%s] g_cis_iso_pool 0x%x size %d count %d",
                   __FUNCTION__,
                   ga_iso_audio_pool,
                   buff_size,
                   p_iso_cfg->max_buffers_per_cis);

    iso_dhm_init(num_complete_handler, rx_handler);
}

void iso_audio_init(const wiced_bt_cfg_isoc_t *p_iso_cfg)
{
    iso_create_pool(p_iso_cfg);
#if ENABLE_LC3_ENCODING
    lc3_codec_reset();
    printf("Audio source: PCM with live LC3 encoding.\n");
#else
    printf("Audio source: Pre-encoded LC3 playback.\n");
#endif
    audio_source_reset();
}

/******************************************************************************
* File Name        : broadcast_sink_isoc.c
*
* Description      : BIS audio data paths and LC3 decoder task integration.
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

#include "iso_data_handler.h"
#include "wiced_bt_cfg.h"

#include "wiced_bt_ga_ascs.h"
#include "wiced_bt_trace.h"
#include "data_types.h"
#include "wiced_bt_version.h"
#include "FreeRTOS.h"
#include "audio_driver_psoc.h"
#include "audio_decoder_task.h"
#include "app_bt_utils.h"
#include "logging.h"

#define MAX_STREAMS_SUPPORTED 2

static audio_decoder_q_data_t audio_decoder_q_data;   // needed for all queue sends
BOOL32 isStreaming = FALSE;                              // needed for SUPPORT_PLC path

typedef struct
{
    wiced_bool_t is_cis;
    uint16_t conn_hdl;
    uint16_t octets_per_frame;
    uint8_t num_of_channels;
    wiced_bool_t b_stream_active;
    uint32_t frame_duration;
    uint32_t sampling_frequency;
    uint32_t num_bis;
} ga_iso_audio_stream_info_t;

static ga_iso_audio_stream_info_t g_stream_info[MAX_STREAMS_SUPPORTED] = { 0 };

/******************************************************************************
 * Function Name: num_complete_handler
 ******************************************************************************
 * Summary:
 *  Trace completed ISO packet counts when trace logging is enabled.
 *
 * Parameters:
 *  conn_hdl : ISO connection handle
 *  num_sent : Number of completed packets
 *
 * Return:
 *  None
 *
*******************************************************************************/
static void num_complete_handler(uint16_t conn_hdl, uint16_t num_sent)
{
    WICED_BT_TRACE("[%s] conn_hdl %d, num_sent %d\n", __FUNCTION__, conn_hdl, num_sent);
}

static void init_stream_info(uint16_t conn_hdl,
    wiced_bool_t is_cis,
    uint32_t sampling_frequency,
    uint16_t octets_per_frame,
    uint32_t frame_duration,
    uint8_t num_of_channels, int8_t num_bis)
{
    WICED_BT_TRACE("[%s] [is_cis %d] [SF %d] [OPF %d] [FD %d] [num_of_channels %d]\n",
        __FUNCTION__,
        is_cis,
        sampling_frequency,
        octets_per_frame,
        frame_duration,
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
            g_stream_info[i].num_bis = num_bis;
            WICED_BT_TRACE("[%s] conn_hdl 0x%x \n", __FUNCTION__, conn_hdl);
            return;
        }
    }
}

static ga_iso_audio_stream_info_t* get_stream_info(uint16_t conn_hdl)
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

    ga_iso_audio_stream_info_t* p_stream_info = get_stream_info(conn_hdl);
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

wiced_result_t iso_audio_setup_data_path(uint16_t conn_hdl, uint16_t direction, wiced_bt_ga_bap_csc_t* p_csc, uint8_t bis_count)
{
    wiced_ble_isoc_data_path_direction_t data_path_dir = WICED_BLE_ISOC_DPD_MAX_DIRECTIONS;
    wiced_ble_isoc_setup_data_path_info_t isoc_setup_data_path_info = {0};
    int numOfChannels = 1;
    wiced_bool_t is_bis = FALSE;

    WICED_BT_TRACE("[%s] conn_hdl [0x%x] dir [%d] p_csc [0x%x]\n", __FUNCTION__, conn_hdl, direction, p_csc);
    if (!p_csc) return WICED_BADARG;

    /* Check that the BIS exists before setting up its data path. */
    is_bis = wiced_ble_isoc_is_bis_created(conn_hdl);
    if (!is_bis)
    {
        return WICED_BADARG;
    }

    WICED_BT_TRACE("[%s] is_bis %d \n", __FUNCTION__, is_bis);

    /* determine num of channels required and configure codec accordingly
    (if not configured default to 1) */
    if (p_csc->audio_channel_allocation)
    {
        numOfChannels = count_set_bits(p_csc->audio_channel_allocation);
        /*if (numOfChannels > ISO_AUDIO_MAX_PARAM_COUNT)
        {
            return WICED_UNSUPPORTED;
        }*/
    }


    data_path_dir = (direction == WICED_BLE_ISOC_DPD_INPUT_BIT) ? WICED_BLE_ISOC_DPD_INPUT : WICED_BLE_ISOC_DPD_OUTPUT;

    WICED_BT_TRACE("[%s] dir %s sampleRate %d sduInterval %d octetsPerFrame %d numOfChannels %d conn_hdl 0x%x\n",
        __FUNCTION__,
        (data_path_dir == WICED_BLE_ISOC_DPD_INPUT) ? "Source" : "Sink",
        p_csc->sampling_frequency,
        p_csc->frame_duration,
        p_csc->octets_per_codec_frame,
        numOfChannels,
        conn_hdl);

    if (!p_csc->sampling_frequency || !p_csc->frame_duration || !p_csc->octets_per_codec_frame) return WICED_ERROR;

      isoc_setup_data_path_info.isoc_conn_hdl    = conn_hdl;
    isoc_setup_data_path_info.data_path_dir    = data_path_dir;
    isoc_setup_data_path_info.data_path_id     = WICED_BLE_ISOC_DPID_HCI;
    isoc_setup_data_path_info.controller_delay = 0;
    isoc_setup_data_path_info.csc_length       = 0;
    isoc_setup_data_path_info.p_csc            = NULL;
    isoc_setup_data_path_info.p_app_ctx        = NULL;

    if (WICED_SUCCESS != wiced_ble_isoc_setup_data_path(&isoc_setup_data_path_info))
    {
        WICED_BT_TRACE(">> [%s]: Error: wiced_ble_isoc_setup_data_path failed\n", __FUNCTION__);
        return WICED_ERROR;
    }

    /* Only OUTPUT (sink) direction is supported for broadcast sink */
    if ((direction != WICED_BLE_ISOC_DPD_OUTPUT_BIT) && (direction != WICED_BLE_ISOC_DPD_INPUT_OUTPUT_BIT))
    {
        WICED_BT_TRACE(">> [%s]: Error: Unsupported ISOC direction '%d'\n", __FUNCTION__, direction);
        return WICED_UNSUPPORTED;
    }

    /* Initialize the LC3 decoder via the decoder task queue (PSoC pattern) */
    audio_decoder_q_data.decoder.config.sampleRate        = p_csc->sampling_frequency;
    audio_decoder_q_data.decoder.config.sduInterval       = p_csc->frame_duration;
    audio_decoder_q_data.decoder.config.octetsPerFrame    = p_csc->octets_per_codec_frame;
    audio_decoder_q_data.decoder.config.sampleWidthInBits = 16;
    audio_decoder_q_data.decoder.config.channelCount      = numOfChannels;
    audio_decoder_q_data.cmd = LC3_DECODER_INIT;

    if (pdTRUE != xQueueSend(audio_decoder_task_q, &audio_decoder_q_data, portMAX_DELAY))
    {
        WICED_BT_TRACE(">> [%s]: Error: Audio Decoder Task Queue Send failed\n", __FUNCTION__);
        CY_ASSERT(0);
    }


    init_stream_info(conn_hdl,
        0,
        p_csc->sampling_frequency,
        p_csc->octets_per_codec_frame,
        p_csc->frame_duration,
        numOfChannels, bis_count);


   audio_driver_init((wiced_ble_isoc_data_path_bit_t) direction, numOfChannels,
                  p_csc->sampling_frequency, p_csc->frame_duration);

    return WICED_SUCCESS;
}

static void rx_handler(uint16_t conn_hdl, uint8_t* p_data, uint32_t length)
{

    uint32_t decoded_data_size = 0;
    /* FIXME: Remove the hardcoded 16-bit PCM sample width. */
    ga_iso_audio_stream_info_t* p_stream_info = get_stream_info(conn_hdl);
    uint8_t *lc3_ptr_l = NULL;
    uint8_t *lc3_ptr_r = NULL;
    uint8_t num_channels = 0;
    if (!p_stream_info)
    {
        /* No stream info => R-channel BIS (raw data path, no decoder).
         * Print first 20 packets raw for inspection. */
        static int r_rx_count = 0;
        if (++r_rx_count <= 20)
        {
            WICED_BT_TRACE("[BIS_R] pkt#%d conn=0x%04x len=%lu raw:", r_rx_count, conn_hdl, (unsigned long)length);
            uint32_t print_len = (length < 16) ? length : 16;
            for (uint32_t i = 0; i < print_len; i++)
                WICED_BT_TRACE(" %02X", p_data[i]);
            WICED_BT_TRACE("\n");
        }
        return;
    }
static int rx_count = 0;
if (++rx_count <= 10)
{
    WICED_BT_TRACE("[RX] pkt#%d len=%lu expected=%u num_ch=%u opf=%u\n",
           rx_count, (unsigned long)length,
           p_stream_info->octets_per_frame * num_channels,
           num_channels, p_stream_info->octets_per_frame);
}
    //TODO: Check if the LC3 codec is initialized.

    // Calculate the decoded PCM frame size from the stream configuration.
    decoded_data_size =
        wiced_bt_ga_bap_get_decoded_data_size(p_stream_info->sampling_frequency, p_stream_info->frame_duration);
    decoded_data_size *= sizeof(int16_t);
    audio_decoder_q_data.decoder.info.expected_decoded_data_len_bytes = decoded_data_size;
    if (p_stream_info->num_bis > 1)
    {
        num_channels = 1;
    }
    else
    {
        num_channels = p_stream_info->num_of_channels;
    }

    // Validate received length against expected octets per frame
    if (length != (p_stream_info->octets_per_frame * num_channels))
    {
#ifndef SUPPORT_PLC
        WICED_BT_TRACE_CRIT("[%s] Expected %d bytes, received %d bytes (channel cnt - %d)",
            __FUNCTION__,
            (p_stream_info->octets_per_frame * num_channels),
            (unsigned long) length,
            num_channels);

            return;

#else
        if (!isStreaming)
        return;

        audio_decoder_q_data.cmd = LC3_DECODER_DECODE_DATA;
        audio_decoder_q_data.decoder.info.left_data_ptr = NULL;
        audio_decoder_q_data.decoder.info.right_data_ptr = NULL;
        audio_decoder_q_data.decoder.info.left_data_len_bytes = p_stream_info->octets_per_frame;
        audio_decoder_q_data.decoder.info.right_data_len_bytes = 0;


        if (p_stream_info->num_bis == 1)
        {
            if (2 == num_channels)
            {
                 audio_decoder_q_data.decoder.info.right_data_len_bytes = p_stream_info->octets_per_frame;
            }
        }
        if (pdTRUE != xQueueSend(audio_decoder_task_q, &audio_decoder_q_data, portMAX_DELAY))
        {
            WICED_BT_TRACE(">> [%s]: Error: Audio Decoder Task Queue Send failed\n", __FUNCTION__);
            CY_ASSERT(0);
        }
        bt_rx_fps_show(SHOW_FPS_SEC);
        return;
#endif

    }

 lc3_ptr_l = get_lc3_buffer_ptr(p_stream_info->octets_per_frame);

    memcpy(lc3_ptr_l, p_data, p_stream_info->octets_per_frame);

    audio_decoder_q_data.cmd = LC3_DECODER_DECODE_DATA;
    audio_decoder_q_data.decoder.info.left_data_ptr = lc3_ptr_l;
    audio_decoder_q_data.decoder.info.left_data_len_bytes = p_stream_info->octets_per_frame;
    audio_decoder_q_data.decoder.info.right_data_ptr = NULL;
    audio_decoder_q_data.decoder.info.right_data_len_bytes = 0;

    if (p_stream_info->num_bis == 1)
    {
        if (2 == num_channels)
        {
             lc3_ptr_r = get_lc3_buffer_ptr(p_stream_info->octets_per_frame);
        memcpy(lc3_ptr_r, (p_data + p_stream_info->octets_per_frame),
               p_stream_info->octets_per_frame);

        audio_decoder_q_data.decoder.info.right_data_ptr = lc3_ptr_r;
        audio_decoder_q_data.decoder.info.right_data_len_bytes = p_stream_info->octets_per_frame;
   }
    }

    isStreaming = TRUE;
    bt_rx_fps_show(SHOW_FPS_SEC);

 if (pdTRUE != xQueueSend(audio_decoder_task_q, &audio_decoder_q_data, portMAX_DELAY))
    {
        WICED_BT_TRACE(">> [%s]: Error: Audio Decoder Task Queue Send failed\n", __FUNCTION__);
        CY_ASSERT(0);
    }
}

void iso_audio_init(const wiced_bt_cfg_isoc_t* p_iso_cfg)
{
    iso_dhm_init(num_complete_handler, rx_handler);
    audio_decoder_q_data.cmd = LC3_DECODER_RESET;   // replace lc3_codec_reset()
    if (pdTRUE != xQueueSend(audio_decoder_task_q, &audio_decoder_q_data, portMAX_DELAY))
    {
        WICED_BT_TRACE(">> [%s]: Audio Decoder Task Queue Send failed\n", __FUNCTION__);
        CY_ASSERT(0);
    }
}

/******************************************************************************
 * Function Name: iso_audio_remove_data_path
 ******************************************************************************
 * Summary: Tears down the ISOC data path for a BIS connection handle.
 *          Deinits the audio driver, frees the LC3 decoder via the task queue,
 *          clears stream info, and removes the BT stack data path.
 *
 * Parameters:
 *  uint16_t                      conn_hdl  : BIS connection handle
 *  wiced_ble_isoc_data_path_bit_t dir      : data path direction bits
 *  uint8_t                       *idx_list : (unused, reserved)
 *
 * Return:
 *  None
 *
*******************************************************************************/
void iso_audio_remove_data_path(uint16_t conn_hdl,
                                wiced_ble_isoc_data_path_bit_t dir,
                                uint8_t *idx_list)
{
    wiced_bool_t is_bis = FALSE;
    ga_iso_audio_stream_info_t *p_stream_info = get_stream_info(conn_hdl);

    if (p_stream_info == NULL)
    {
        WICED_BT_TRACE(">> [%s]: stream info not found for conn_hdl 0x%x\n", __FUNCTION__, conn_hdl);
        return;
    }

    WICED_BT_TRACE("[%s] dir %d conn_hdl 0x%x num_of_channels %d\n",
                   __FUNCTION__, dir, conn_hdl, p_stream_info->num_of_channels);

    /* Deinit the I2S audio driver */
    audio_driver_deinit(dir);

    /* Deinit stream info entry so conn_hdl slot is free for reuse */
    deinit_stream_info(conn_hdl);

    /*
     * Free the LC3 decoder via the task queue (PSoC pattern).
     * Broadcast sink is OUTPUT only, so only handle the sink/output path.
     * No encoder release needed (no INPUT path for a broadcast sink).
     */
    if (WICED_BLE_ISOC_DPD_OUTPUT_BIT & dir)
    {
        audio_decoder_q_data.cmd = LC3_DECODER_FREE;

        if (pdTRUE != xQueueSend(audio_decoder_task_q, &audio_decoder_q_data, portMAX_DELAY))
        {
            WICED_BT_TRACE(">> [%s]: Error: Audio Decoder Task Queue Send failed\n", __FUNCTION__);
            CY_ASSERT(0);
        }
    }

    isStreaming = FALSE;

    /* Verify BIS is still established before asking the stack to remove the data path */
    is_bis = wiced_ble_isoc_is_bis_created(conn_hdl);
    if (!is_bis)
    {
        WICED_BT_TRACE("[%s] BIS already gone for conn_hdl 0x%x, skipping stack remove\n",
                       __FUNCTION__, conn_hdl);
        return;
    }

    if (!wiced_ble_isoc_remove_data_path(conn_hdl, dir, NULL))
        WICED_BT_TRACE_CRIT("[%s] No active data path to remove for conn_hdl 0x%x\n",
                             __FUNCTION__, conn_hdl);


}

void iso_audio_stop_stream(uint16_t conn_hdl)
{
    ga_iso_audio_stream_info_t* p_stream_info = get_stream_info(conn_hdl);
    p_stream_info->b_stream_active = 0;
    isStreaming = FALSE;
}

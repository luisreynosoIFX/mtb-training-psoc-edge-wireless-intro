/******************************************************************************
* File Name        : broadcast_source_bis.c
*
* Description      : Broadcast Source BIS handling.
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
/* BT Stack includes */
#include <stdio.h>
#include <string.h>
#include "wiced_bt_trace.h"

/* App Library includes */
#include "broadcast_source_iso_audio.h"
#include "wiced_bt_isoc.h"
#include "wiced_bt_adv_scan_extended.h"
#include "wiced_bt_adv_scan_periodic.h"

/* Application includes */
#include "broadcast_source_bis.h"
#include "le_audio_pbp.h"
#include "logging.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define SUB_GROUP_CNT 1
#define BIS_CNT 1
#define CONTROLLER_BUSY 0x3A
#define BROADCASTING_DEVICE 0x0885

/*******************************************************************************
* Extern Variables
*******************************************************************************/
extern const wiced_bt_cfg_settings_t broadcast_source_cfg_settings;

/*******************************************************************************
* Global Variables
*******************************************************************************/
broadcast_source_cb_t g_broadcast_source_cb = {
    .big_handle = 1,
    .adv_handle = 1,
    .b_encryption = TRUE,
    .base =
        {
            .broadcast_id = 0x123456,
            .sub_group_cnt = 1,
            .sub_group[0] = {.codec_id =
                                 {
                                     .coding_format = LC3_CODEC_ID,
                                 },
                             .csc =
                                 {
                                     .sampling_frequency = 48000,
                                     .frame_duration = 10000,
                                     .octets_per_codec_frame = 100,
                                     .audio_channel_allocation = 1,
                                 },
                             .metadata =
                                 {
                                     .streaming_audio_ctx = 2,
                                 },
                             .bis_cnt = BIS_CNT,
                             .bis_config[0] =
                                 {
                                     .bis_idx = 1,
                                 }},
        },
};

/*******************************************************************************
* Function Name: broadcast_source_bis_isoc_cb
********************************************************************************
* Summary:
*   Handle BIG and data path events to control Broadcast Source streaming.
*
* Parameters:
*   event : ISO event type.
*   p_ed  : Event-specific data.
*
* Return:
*   None
*******************************************************************************/
static void broadcast_source_bis_isoc_cb(wiced_ble_isoc_event_t event, wiced_ble_isoc_event_data_t *p_ed)
{
    wiced_ble_isoc_terminated_evt_t *p_big_terminated = NULL;
    wiced_ble_isoc_create_big_cmpl_evt_t *p_create_big_sts = NULL;
    broadcast_source_cb_t *p_big = &g_broadcast_source_cb;
    wiced_result_t res = WICED_ERROR;

    WICED_BT_TRACE("[%s] event %d ", __FUNCTION__, event);

    switch (event)
    {
        case WICED_BLE_ISOC_BIG_CREATED_EVT:
            p_create_big_sts = &p_ed->create_big;

            if (p_create_big_sts->sync_data.status)
            {
                  printf(">> Error: Broadcast creation failed (0x%02x).\n",
                      p_create_big_sts->sync_data.status);
                return;
            }

            /* Map BIS index and bis_conn_handle */
            p_big->bis_conn_id_count = p_create_big_sts->sync_data.num_bis;
            memcpy(p_big->bis_conn_id_list,
                   p_create_big_sts->sync_data.bis_conn_hdl_list,
                   p_big->bis_conn_id_count * sizeof(uint16_t));

            /* Set up the input data path for the first BIS stream. */
            res = iso_audio_setup_data_path(p_big->bis_conn_id_list[0],
                                            WICED_BLE_ISOC_DPD_INPUT_BIT,
                                            &p_big->base.sub_group[0].csc);
            if (res)
            {
                printf(">> Error: Broadcast audio data path request failed (0x%02x).\n", res);
            }
            break;

        case WICED_BLE_ISOC_BIG_TERMINATED_EVT:
            p_big_terminated = &p_ed->terminate_big;
            printf("Audio broadcasting stopped (reason 0x%02x).\n", p_big_terminated->reason);

            WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_big->base.state);
            break;

        case WICED_BLE_ISOC_DATA_PATH_SETUP_EVT:
            WICED_BT_TRACE("[DATA_PATH_SETUP] status=%d conn_hdl=0x%x\n",
                   (int)p_ed->datapath.status, (unsigned int)p_ed->datapath.conn_hdl);
            if (p_ed->datapath.status)
            {
                  printf(">> Error: Broadcast audio data path setup failed (0x%02x).\n",
                      p_ed->datapath.status);
                return;
            }

            p_big->base.state = BAP_BROADCAST_STATE_STREAMING;
            WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_big->base.state);

            /* start streaming as broadcast source */
            iso_audio_start_stream(p_ed->datapath.conn_hdl);
            break;

        case WICED_BLE_ISOC_DATA_PATH_REMOVED_EVT:
            if (p_ed->datapath.status)
            {
                WICED_BT_TRACE_CRIT("[%s] Data path removal not successful\n", __FUNCTION__);
                return;
            }

            res = wiced_ble_isoc_central_terminate_big(p_big->big_handle, 0);
            iso_audio_stop_stream(p_ed->datapath.conn_hdl);
            (void)res;
            break;

        default:
            WICED_BT_TRACE("[%s] Unknown event %d ", __FUNCTION__, event);
            break;
    }
}

/*******************************************************************************
* Function Name: broadcast_source_bis_init
********************************************************************************
* Summary:
*   Initialize ISO event handling and the audio transmit resources.
*
* Parameters:
*   p_isoc_cfg : ISO resource configuration.
*
* Return:
*   None
*******************************************************************************/
void broadcast_source_bis_init(wiced_bt_cfg_isoc_t *p_isoc_cfg)
{
    wiced_ble_isoc_cfg_t ble_isoc_cfg = { .max_cis = 0, .max_bis = p_isoc_cfg->max_big_count };
    wiced_ble_isoc_init(&ble_isoc_cfg, broadcast_source_bis_isoc_cb);
    iso_audio_init(p_isoc_cfg);
}

void broadcast_source_bis_start_stream(wiced_bt_ga_bap_stream_config_t *p_stream_config)
{
    wiced_ble_isoc_create_big_param_t create_big_params = {0};
    broadcast_source_cb_t *p_big = &g_broadcast_source_cb;

    /* validate BASE state */
    if (BAP_BROADCAST_STATE_CONFIGURED != p_big->base.state) return;

    /* create BIG */
    create_big_params.big_handle = p_big->big_handle;
    create_big_params.adv_handle = p_big->adv_handle;

    for (int i = 0; i < p_big->base.sub_group_cnt; ++i)
    {
        create_big_params.num_bis += p_big->base.sub_group[i].bis_cnt;
    }

    create_big_params.sdu_interval = p_stream_config->sdu_interval;
    if (p_big->base.sub_group[0].csc.audio_channel_allocation ==
        (BAP_AUDIO_LOCATION_FRONT_LEFT | BAP_AUDIO_LOCATION_FRONT_RIGHT))
        create_big_params.max_sdu = p_big->base.sub_group[0].csc.octets_per_codec_frame * 2;
    else
        create_big_params.max_sdu = p_big->base.sub_group[0].csc.octets_per_codec_frame;
    create_big_params.max_trans_latency = p_stream_config->max_transport_latency;
    create_big_params.rtn = p_stream_config->retransmission_number;

    create_big_params.phy = WICED_BLE_ISOC_LE_2M_PHY;
    create_big_params.packing = WICED_BLE_ISOC_SEQUENTIAL_PACKING;
    create_big_params.framing = p_stream_config->framing;

    create_big_params.encrypt = p_big->b_encryption;

    if (p_big->b_encryption)
        memcpy(create_big_params.broadcast_code, p_big->broadcast_code, BAP_BROADCAST_CODE_SIZE);
    else
        memset(create_big_params.broadcast_code, 0, BAP_BROADCAST_CODE_SIZE);

    WICED_BT_TRACE("[%s] broadcast_code %A \n", __FUNCTION__, create_big_params.broadcast_code, 16);

    WICED_BT_TRACE("[BIG_CREATE] big_hdl=%d adv_hdl=%d num_bis=%d sdu_interval=%lu max_sdu=%d "
           "max_latency=%d rtn=%d phy=%d packing=%d framing=%d encrypt=%d\n",
           (int)create_big_params.big_handle,
           (int)create_big_params.adv_handle,
           (int)create_big_params.num_bis,
           (unsigned long)create_big_params.sdu_interval,
           (int)create_big_params.max_sdu,
           (int)create_big_params.max_trans_latency,
           (int)create_big_params.rtn,
           (int)create_big_params.phy,
           (int)create_big_params.packing,
           (int)create_big_params.framing,
           (int)create_big_params.encrypt);

    wiced_result_t res = wiced_ble_isoc_central_create_big(&create_big_params);
    WICED_BT_TRACE("[%s] res %x \n", __FUNCTION__, res);

    if (res == CONTROLLER_BUSY)
    {
        /* controller is busy, create big again */
        res = wiced_ble_isoc_central_create_big(&create_big_params);
        WICED_BT_TRACE("[%s] create big retry %x \n", __FUNCTION__, res);
    }
    if (res != WICED_SUCCESS)
    {
        printf(">> Error: Broadcast creation request failed (0x%02x).\n", res);
    }
}

le_audio_bap_broadcast_base_t *broadcast_source_bis_update_config(uint32_t broadcast_id,
                                                                     uint8_t *broadcast_code,
                                                                     uint8_t bis_cnt,
                                                                     uint32_t channel_counts,
                                                                     uint32_t sampling_freq,
                                                                     uint32_t frame_duration,
                                                                     uint16_t octets_per_codec_frame,
                                                                     wiced_bool_t enable_encryption)
{
    broadcast_source_cb_t *p_big = &g_broadcast_source_cb;

    p_big->base.presentation_delay = 0;
    p_big->base.broadcast_id = broadcast_id;

    p_big->b_encryption = enable_encryption;
    if (p_big->b_encryption) memcpy(p_big->broadcast_code, broadcast_code, BAP_BROADCAST_CODE_SIZE);

    p_big->base.sub_group[0].csc.sampling_frequency = sampling_freq;
    p_big->base.sub_group[0].csc.frame_duration = frame_duration;
    p_big->base.sub_group[0].csc.octets_per_codec_frame = octets_per_codec_frame;
    p_big->base.sub_group[0].bis_cnt = bis_cnt;

    p_big->base.sub_group[0].csc.audio_channel_allocation = BAP_AUDIO_LOCATION_FRONT_LEFT;
    if (2 == channel_counts) p_big->base.sub_group[0].csc.audio_channel_allocation |= BAP_AUDIO_LOCATION_FRONT_RIGHT;

    WICED_BT_TRACE("[%s] broadcast_code %A \n", __FUNCTION__, p_big->broadcast_code, BAP_BROADCAST_CODE_SIZE);

    return &p_big->base;
}

static void broadcast_source_get_metadata(le_audio_public_broadcast_t *p_pub_br)
{
    uint8_t program_info[] = "IFX_BROADCAST_DEMO";
    p_pub_br->name_length = strlen((char *)broadcast_source_cfg_settings.device_name);
    p_pub_br->broadcast_name = broadcast_source_cfg_settings.device_name;
    p_pub_br->appearance_value = BROADCASTING_DEVICE;
    p_pub_br->program_info_size = sizeof(program_info);
    p_pub_br->program_info = program_info;
    p_pub_br->metadata_length = 0;
}

wiced_result_t broadcast_source_build_broadcast_adv_data(uint8_t adv_sid,
                                                         uint32_t broadcast_id,
                                                         uint32_t sampling_frequency,
                                                         uint16_t frame_duration,
                                                         wiced_bool_t encryption,
                                                         uint8_t *p_adv_len,
                                                         uint8_t *p_ext_adv)
{
    uint8_t len = 0;
    uint32_t pub_br_len = 0;
    le_audio_public_broadcast_t pub_br;
    wiced_result_t ret_sts =
        le_audio_bap_build_broadcast_annoncement_adv_data(p_ext_adv, &len, broadcast_id);
    if (ret_sts != WICED_SUCCESS) return ret_sts;
    pub_br.encryption = encryption;
    pub_br.sampling_frequency = sampling_frequency;
    pub_br.frame_duration = frame_duration;
    broadcast_source_get_metadata(&pub_br);
    ret_sts = le_audio_pbp_build_adv_data(p_ext_adv, len, &pub_br, &pub_br_len);
    *p_adv_len = len + pub_br_len;

    return WICED_SUCCESS;
}

wiced_result_t broadcast_source_bis_configure_stream(uint32_t broadcast_id,
                                                     uint8_t *broadcast_code,
                                                     uint8_t bis_cnt,
                                                     uint32_t channel_counts,
                                                     uint32_t sampling_freq,
                                                     uint32_t frame_duration,
                                                     uint16_t octets_per_codec_frame,
                                                     wiced_bool_t enable_encryption)
{
    wiced_result_t ret_sts = WICED_ERROR;
    le_audio_bap_broadcast_base_t *p_base = NULL;
    uint8_t adv_sid = 1;

    p_base = broadcast_source_bis_update_config(broadcast_id,
                                                broadcast_code,
                                                bis_cnt,
                                                channel_counts,
                                                sampling_freq,
                                                frame_duration,
                                                octets_per_codec_frame,
                                                enable_encryption);
    if (!p_base) return ret_sts;

    uint8_t adv_len = 0;
    uint8_t p_ext_adv[250] = {0};
    ret_sts = broadcast_source_build_broadcast_adv_data(adv_sid,
                                                        broadcast_id,
                                                        sampling_freq,
                                                        frame_duration,
                                                        enable_encryption,
                                                        &adv_len,
                                                        p_ext_adv);
    if (ret_sts == WICED_SUCCESS)
    {
        ret_sts = le_audio_bap_broadcast_configure(adv_sid, p_base, p_ext_adv, adv_len);
        if (WICED_SUCCESS == ret_sts)
        {
            p_base->state = BAP_BROADCAST_STATE_CONFIGURED;
                 printf("Broadcast: %s | ID: 0x%06lx | %lu Hz | %lu channel(s)\n",
                     (const char *)broadcast_source_cfg_settings.device_name,
                     (unsigned long)broadcast_id, (unsigned long)sampling_freq,
                     (unsigned long)channel_counts);
            WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_base->state);
        }
    }
    return ret_sts;
}

wiced_result_t broadcast_source_bis_disable_stream(void)
{
    broadcast_source_cb_t *p_big = &g_broadcast_source_cb;
    uint8_t lc3_index = 0;

    /* validate BASE state */
    if (BAP_BROADCAST_STATE_STREAMING != p_big->base.state) return WICED_ERROR;

    iso_audio_remove_data_path(p_big->bis_conn_id_list[0], WICED_BLE_ISOC_DPD_INPUT_BIT, &lc3_index);

    return WICED_SUCCESS;
}

wiced_result_t broadcast_source_bis_release_stream(void)
{
    wiced_ble_ext_adv_duration_config_t duration_cfg;
    broadcast_source_cb_t *p_big = &g_broadcast_source_cb;
    wiced_result_t ret_sts = WICED_ERROR;

    wiced_ble_isoc_central_terminate_big(p_big->adv_handle, 0);

    duration_cfg.adv_handle = p_big->adv_handle;
    duration_cfg.adv_duration = 0;
    duration_cfg.max_ext_adv_events = 0;

    ret_sts = wiced_ble_ext_adv_enable(FALSE, 1, &duration_cfg);
    ret_sts = wiced_ble_padv_enable_adv(p_big->adv_handle, FALSE);

    p_big->base.state = BAP_BROADCAST_STATE_IDLE;
    WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_big->base.state);

    return ret_sts;
}

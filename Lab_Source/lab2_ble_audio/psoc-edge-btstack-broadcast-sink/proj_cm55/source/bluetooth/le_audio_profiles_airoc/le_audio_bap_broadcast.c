/*
 * (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
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
 */

#include "wiced_bt_ble.h"
#include "wiced_bt_ga_ascs.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_isoc.h"
#include "wiced_bt_trace.h"
#include "wiced_memory.h"

#include "le_audio_bap_broadcast.h"

#define AD_FLAG_SIZE 2
#define MAX_SUPPORTED_BASE_SIZE 252
#define MAX_DEV_NAME_LEN 50

wiced_result_t bap_priv_start_broadcast_audio_announcements(uint8_t adv_sid,
                                                            uint32_t broadcast_id,
                                                            uint8_t *p_ext_adv_data,
                                                            uint8_t adv_size)
{
    wiced_ble_ext_adv_duration_config_t duration_cfg;

    if (!adv_sid || !broadcast_id) return WICED_BADARG;

     wiced_ble_ext_adv_params_t params = {
        .event_properties = WICED_BLE_EXT_ADV_EVENT_PROPERTY_INCLUDE_TX_POWER,
        .primary_adv_int_min = 48,
        .primary_adv_int_max = 48,
        .primary_adv_channel_map = (BTM_BLE_ADVERT_CHNL_37 | BTM_BLE_ADVERT_CHNL_38 | BTM_BLE_ADVERT_CHNL_39),
        .own_addr_type = BLE_ADDR_PUBLIC,
        .peer_addr_type = BLE_ADDR_PUBLIC,
        .peer_addr = {0, 0, 0, 0, 0, 0},
        .adv_filter_policy = BTM_BLE_ADV_POLICY_ACCEPT_CONN_AND_SCAN,
        .adv_tx_power = 0x7f,
        .primary_adv_phy = WICED_BLE_EXT_ADV_PHY_1M,
        .secondary_adv_max_skip = 0,
        .secondary_adv_phy = WICED_BLE_EXT_ADV_PHY_1M,
        .adv_sid = 1,
        .scan_request_not = WICED_BLE_EXT_ADV_SCAN_REQ_NOTIFY_ENABLE,
        .primary_phy_opts = 0,
        .secondary_phy_opts = 0};

    wiced_ble_ext_adv_set_params(adv_sid, &params);

    // Set adv data in LTV format
    wiced_ble_ext_adv_set_adv_data(adv_sid, adv_size, p_ext_adv_data);

    duration_cfg.adv_handle = adv_sid;
    duration_cfg.adv_duration = 0;
    duration_cfg.max_ext_adv_events = 0;

    // Start adv
    wiced_ble_ext_adv_enable(TRUE, 1, &duration_cfg);

    return WICED_SUCCESS;
}

wiced_result_t le_audio_bap_build_broadcast_annoncement_adv_data(uint8_t *p_data, uint8_t *len, uint32_t broadcast_id)
{
    if (!broadcast_id)
        return WICED_ERROR;
    BAP_BROADCAST_TRACE("[%s] broadcast id %d", __FUNCTION__, broadcast_id);
    uint8_t *p_adv_data = p_data;

    UINT8_TO_STREAM(p_adv_data, AD_FLAG_SIZE);
    UINT8_TO_STREAM(p_adv_data, BTM_BLE_ADVERT_TYPE_FLAG);
    UINT8_TO_STREAM(p_adv_data, BTM_BLE_GENERAL_DISCOVERABLE_FLAG);

    // update WICED_BT_UUID_BROADCAST_AUDIO_ANNOUNCEMENT data
    UINT8_TO_STREAM(p_adv_data, BROADCAST_AUDIO_ANNOUNCEMENT_SIZE);
    UINT8_TO_STREAM(p_adv_data, BTM_BLE_ADVERT_TYPE_SERVICE_DATA);
    UINT16_TO_STREAM(p_adv_data, WICED_BT_UUID_BROADCAST_AUDIO_ANNOUNCEMENT);
    ARRAY_TO_STREAM(p_adv_data, &broadcast_id, 3);

    *len = p_adv_data - p_data;
    return WICED_SUCCESS;
}

uint8_t le_audio_bap_build_base_data(le_audio_bap_broadcast_base_t *p_base, uint8_t *p_dst, uint32_t dst_len)
{
    uint8_t *p_dst_cache = p_dst;
    uint8_t *p_adv_data_len = NULL;
    uint8_t bytes_written = 0;

    // validate params
    if (!p_base || !p_dst)
        return 0;

    p_adv_data_len = p_dst++;

    UINT8_TO_STREAM(p_dst, BTM_BLE_ADVERT_TYPE_SERVICE_DATA);
    UINT16_TO_STREAM(p_dst, WICED_BT_UUID_BASIC_AUDIO_ANNOUNCEMENT);

    ARRAY_TO_STREAM(p_dst, &p_base->presentation_delay, PRESENTATION_DELAY_LENGTH);
    UINT8_TO_STREAM(p_dst, p_base->sub_group_cnt);

    for (size_t i = 0; i < p_base->sub_group_cnt; i++) {
        UINT8_TO_STREAM(p_dst, p_base->sub_group[i].bis_cnt);
        UINT8_TO_STREAM(p_dst, p_base->sub_group[i].codec_id.coding_format);
        UINT16_TO_STREAM(p_dst, p_base->sub_group[i].codec_id.company_id);
        UINT16_TO_STREAM(p_dst, p_base->sub_group[i].codec_id.vendor_specific_codec_id);

        p_dst = wiced_bt_ga_bap_fill_csc(p_dst, &p_base->sub_group[i].csc);
        p_dst = wiced_bt_ga_bap_fill_metadata(p_dst, &p_base->sub_group[i].metadata);

        for (size_t j = 0; j < p_base->sub_group[i].bis_cnt; j++) {
            BAP_BROADCAST_TRACE("[%s] j %d bis_idx %d\n", __FUNCTION__, j, p_base->sub_group[i].bis_config[j].bis_idx);
            UINT8_TO_STREAM(p_dst, p_base->sub_group[i].bis_config[j].bis_idx);
            p_dst = wiced_bt_ga_bap_fill_csc(p_dst, &p_base->sub_group[i].bis_config[j].bis_csc);
        }
    }

    bytes_written = (p_dst - p_dst_cache);
    if (bytes_written > dst_len) {
        BAP_BROADCAST_TRACE_CRIT("[%s] possible memory corruption.. wrote %d bytes beyond allocated memory\n",
                                 __FUNCTION__,
                                 (bytes_written - dst_len));
        return 0;
    }

    /* length field should not include the its own length, hence -1 */
    *p_adv_data_len = (bytes_written - 1);

    return bytes_written;
}

wiced_result_t bap_priv_start_basic_audio_announcements(uint8_t adv_sid, le_audio_bap_broadcast_base_t *p_base)
{
    uint8_t *p_basic_audio_announcement = NULL;
    uint8_t base_data_size = 0;

    if (!p_base)
        return WICED_BT_BADARG;

    p_basic_audio_announcement = wiced_bt_get_buffer(MAX_SUPPORTED_BASE_SIZE);
    if (!p_basic_audio_announcement)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    memset(p_basic_audio_announcement, 0, MAX_SUPPORTED_BASE_SIZE);
    base_data_size = le_audio_bap_build_base_data(p_base, p_basic_audio_announcement, MAX_SUPPORTED_BASE_SIZE);
    if (!base_data_size)
        return WICED_BT_BADARG;

    {
        wiced_ble_padv_params_t params = {.adv_int_min = 200, .adv_int_max = 200, .adv_properties = 0};
        wiced_ble_padv_set_adv_params(adv_sid, &params);
        wiced_ble_padv_set_adv_data(adv_sid, base_data_size, p_basic_audio_announcement);
    }

    wiced_bt_free_buffer(p_basic_audio_announcement);

    wiced_ble_padv_enable_adv(adv_sid, TRUE);

    return WICED_BT_SUCCESS;
}

wiced_result_t bap_priv_update_basic_audio_announcements(uint8_t adv_sid, le_audio_bap_broadcast_base_t *p_base)
{
    uint8_t base_data_size = 0;
    uint8_t buff[MAX_SUPPORTED_BASE_SIZE] = {0};

    if (!adv_sid || !p_base) return WICED_BT_BADARG;

    base_data_size = le_audio_bap_build_base_data(p_base, buff, MAX_SUPPORTED_BASE_SIZE);
    if (!base_data_size) return WICED_BT_BADARG;

    wiced_ble_padv_set_adv_data(adv_sid, base_data_size, buff);
    return WICED_BT_SUCCESS;
}

wiced_result_t le_audio_bap_broadcast_configure(uint8_t adv_sid, le_audio_bap_broadcast_base_t *p_base, uint8_t *p_ext_adv_data, uint8_t adv_size)
{
    wiced_result_t res = WICED_ERROR;

    /* Validate BASE state */
    if (!p_base || BAP_BROADCAST_STATE_IDLE != p_base->state) return WICED_ERROR;

    res = bap_priv_start_broadcast_audio_announcements(adv_sid, p_base->broadcast_id, p_ext_adv_data, adv_size);
    if (WICED_SUCCESS != res) return res;

    res = bap_priv_start_basic_audio_announcements(adv_sid, p_base);
    if (WICED_SUCCESS != res) return res;

    return res;
}

wiced_result_t le_audio_bap_broadcast_reconfigure(uint8_t adv_sid, le_audio_bap_broadcast_base_t *p_base)
{
    wiced_result_t res = WICED_ERROR;

    /* Validate BASE state */
    if (BAP_BROADCAST_STATE_CONFIGURED != p_base->state) return WICED_ERROR;

    res = bap_priv_update_basic_audio_announcements(adv_sid, p_base);
    if (WICED_SUCCESS != res) return res;

    return res;
}

wiced_result_t le_audio_bap_broadcast_update_metadata(uint8_t adv_sid, le_audio_bap_broadcast_base_t *p_base)
{
    wiced_result_t res = WICED_ERROR;

    /* Validate BASE state */
    if (BAP_BROADCAST_STATE_STREAMING != p_base->state) return WICED_ERROR;

    res = bap_priv_update_basic_audio_announcements(adv_sid, p_base);
    if (WICED_SUCCESS != res) return res;

    return res;
}

uint8_t* le_audio_bap_broadcast_is_basic_announcement(uint8_t *p_adv_data, uint16_t adv_len, uint8_t *p_base_len)
{
    uint16_t base_len;
    wiced_bt_adv_ctx_t ctx = {.p_adv = p_adv_data, .adv_len = adv_len, .offset = 0};
    uint8_t *p_base = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, &base_len);

    while (p_base)
    {
        uint8_t *p = p_base;
        uint16_t u16;
        int found;

        STREAM_TO_UINT16(u16, p);
        found = (u16 == WICED_BT_UUID_BASIC_AUDIO_ANNOUNCEMENT)?1:0;
        WICED_BT_TRACE("[%s] 0x%04x len %d", __FUNCTION__, u16, base_len);
        if (found)
        {
            *p_base_len = (uint8_t)base_len;
            return p_base;
        }
        p_base = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, &base_len);
    }

    return NULL;
}

wiced_result_t le_audio_bap_broadcast_parse_base_info(uint8_t *p_base_stream,
                                                         uint8_t base_stream_len,
                                                         le_audio_bap_broadcast_base_t *p_base)
{
#define WICED_BT_GA_BAP_MIN_SUBGROUP_LEN 8
    wiced_result_t res = WICED_SUCCESS;
    uint8_t csc_length = 0;
    uint8_t metadata_length = 0;
    uint16_t uuid;

    // TODO: validate stream_len, max_sub_group_supported, max_bis_per_group
    STREAM_TO_UINT16(uuid, p_base_stream);
    STREAM_TO_ARRAY(&p_base->presentation_delay, p_base_stream, PRESENTATION_DELAY_LENGTH);
    base_stream_len -= PRESENTATION_DELAY_LENGTH;
    STREAM_TO_UINT8(p_base->sub_group_cnt, p_base_stream);
    base_stream_len--;

    for (size_t i = 0; i < p_base->sub_group_cnt && base_stream_len >= WICED_BT_GA_BAP_MIN_SUBGROUP_LEN; i++)
    {
        STREAM_TO_UINT8(p_base->sub_group[i].bis_cnt, p_base_stream);
        STREAM_TO_UINT8(p_base->sub_group[i].codec_id.coding_format, p_base_stream);
        STREAM_TO_UINT16(p_base->sub_group[i].codec_id.company_id, p_base_stream);
        STREAM_TO_UINT16(p_base->sub_group[i].codec_id.vendor_specific_codec_id, p_base_stream);
        STREAM_TO_UINT8(csc_length, p_base_stream);
        base_stream_len -= WICED_BT_GA_BAP_MIN_SUBGROUP_LEN;
        if (base_stream_len < csc_length)
        {
            return WICED_BT_ERROR;
        }
        memset(&p_base->sub_group[i].csc, 0, sizeof(wiced_bt_ga_bap_csc_t));
        wiced_bt_ga_bap_parse_csc(p_base_stream, csc_length, &p_base->sub_group[i].csc);
        base_stream_len -= csc_length;
        p_base_stream += csc_length;

        STREAM_TO_UINT8(metadata_length, p_base_stream);
        if (base_stream_len < metadata_length)
        {
            return WICED_BT_ERROR;
        }

        wiced_bt_ga_bap_get_metadata(p_base_stream, metadata_length, &p_base->sub_group[i].metadata, NULL, NULL);
        base_stream_len -= metadata_length;
        p_base_stream += metadata_length;

        for (size_t j = 0; j < p_base->sub_group[i].bis_cnt; j++)
        {
            WICED_MEMCPY(&p_base->sub_group[i].bis_config[j].bis_csc,
                         &p_base->sub_group[i].csc,
                         sizeof(wiced_bt_ga_bap_csc_t));
            if (base_stream_len < 2)
            {
                return WICED_BT_ERROR;
            }
            STREAM_TO_UINT8(p_base->sub_group[i].bis_config[j].bis_idx, p_base_stream);
            STREAM_TO_UINT8(csc_length, p_base_stream);
            base_stream_len -= 2;
            if (base_stream_len < csc_length)
            {
                return WICED_BT_ERROR;
            }
            wiced_bt_ga_bap_parse_csc(p_base_stream, csc_length, &p_base->sub_group[i].bis_config[j].bis_csc);
            base_stream_len -= csc_length;
            p_base_stream += csc_length;
        }
    }
    UNUSED_VARIABLE(uuid);
    return res;
}

wiced_bool_t le_audio_bap_broadcast_is_broadcast_announcement(uint16_t adv_len, uint8_t *p_adv_data, uint32_t *p_br_id)
{
    uint8_t *p_data = NULL;
    uint16_t uuid = 0;
    uint16_t adv_entry_length = 0;

    wiced_bt_adv_ctx_t ctx = {
       .adv_len = adv_len,
       .p_adv = p_adv_data,
       .offset = 0
    };
    p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, & adv_entry_length);
    if (p_data == NULL || adv_entry_length == 0) return FALSE;
    while (adv_entry_length) /*if no entry is found adv_entry_length will be 0*/
    {
        /* -1 for type */
        if (p_data == NULL || adv_entry_length != BROADCAST_AUDIO_ANNOUNCEMENT_SIZE - 1) return FALSE;

        /* Get Broadcast ID */
        STREAM_TO_UINT16(uuid, p_data);
        STREAM_TO_ARRAY(p_br_id, p_data, 3);
        if (uuid == WICED_BT_UUID_BROADCAST_AUDIO_ANNOUNCEMENT)
        {
            return TRUE;
        }
        else
        {
            p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, & adv_entry_length);
        }
    }
    return FALSE;
}

wiced_result_t le_audio_bap_broadcast_start_solicitation_requests(uint8_t adv_sid,
                                                                     char *dev_name,
                                                                     uint8_t name_length)
{
    wiced_ble_ext_adv_duration_config_t duration_cfg;

    // +1 for length itself
    uint8_t solicitation_req[100] = {0};
    uint8_t *p_ext_adv_data = solicitation_req;

    if (name_length > MAX_DEV_NAME_LEN) name_length = MAX_DEV_NAME_LEN;
    if (!adv_sid) return WICED_BADARG;

    wiced_ble_ext_adv_params_t params = {
        .event_properties = WICED_BLE_EXT_ADV_EVENT_PROPERTY_CONNECTABLE_ADV,
        .primary_adv_int_min = 1000,
        .primary_adv_int_max = 1000,
        .primary_adv_channel_map = (BTM_BLE_ADVERT_CHNL_37 | BTM_BLE_ADVERT_CHNL_38 | BTM_BLE_ADVERT_CHNL_39),
        .own_addr_type = BLE_ADDR_PUBLIC,
        .peer_addr_type = BLE_ADDR_PUBLIC,
        .peer_addr = {0, 0, 0, 0, 0, 0},
        .adv_filter_policy = BTM_BLE_ADV_POLICY_ACCEPT_CONN_AND_SCAN,
        .adv_tx_power = 0x7f,
        .primary_adv_phy = WICED_BLE_EXT_ADV_PHY_1M,
        .secondary_adv_max_skip = 0,
        .secondary_adv_phy = WICED_BLE_EXT_ADV_PHY_1M,
        .adv_sid = 1,
        .scan_request_not = WICED_BLE_EXT_ADV_SCAN_REQ_NOTIFY_ENABLE,
        .primary_phy_opts = 0,
        .secondary_phy_opts = 0};

    wiced_ble_ext_adv_set_params(adv_sid, &params);

    UINT8_TO_STREAM(p_ext_adv_data, AD_FLAG_SIZE);
    UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_ADVERT_TYPE_FLAG);
    UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED);

    UINT8_TO_STREAM(p_ext_adv_data, SOLICITATION_REQ_SIZE);
    UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_ADVERT_TYPE_SERVICE_DATA);
    UINT16_TO_STREAM(p_ext_adv_data, WICED_BT_UUID_BROADCAST_AUDIO_SCAN);
    UINT8_TO_STREAM(p_ext_adv_data, name_length + 1);
    UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_ADVERT_TYPE_NAME_COMPLETE);
    ARRAY_TO_STREAM(p_ext_adv_data, dev_name, name_length);

    // Set adv data in LTV format
    wiced_ble_ext_adv_set_adv_data(adv_sid, (p_ext_adv_data - solicitation_req), solicitation_req);

    duration_cfg.adv_handle = adv_sid;
    duration_cfg.adv_duration = 0;
    duration_cfg.max_ext_adv_events = 0;

    // Start adv
    wiced_ble_ext_adv_enable(TRUE, 1, &duration_cfg);

    return WICED_SUCCESS;
}

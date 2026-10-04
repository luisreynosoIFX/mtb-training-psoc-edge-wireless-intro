/******************************************************************************
* File Name        : broadcast_sink_bis.h
*
* Description      : This file is the public interface of broadcast_sink_bis.c.
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

#include <stdbool.h>
#include "wiced_bt_types.h"
#include "le_audio_bap_broadcast.h"
#include "wiced_bt_ga_bap.h"

typedef struct
{
    // app info
    wiced_bool_t in_use;
    wiced_bt_device_address_t bd_addr;
    uint8_t big_handle;
    uint8_t adv_handle;
    wiced_bool_t b_base_updated;
    wiced_bool_t b_biginfo_updated;
    uint8_t *p_periodic_adv_data;
    uint8_t periodic_adv_data_offset;
    uint8_t base_len;

    // controller info
    wiced_ble_padv_sync_handle_t sync_handle;
    wiced_bool_t sync_in_progress;
    uint8_t number_of_subevents; // for sink only (received in BIGInfo)
    uint8_t bis_conn_id_count;
    uint16_t bis_conn_id_list[BROADCAST_MAX_BIS_PER_SUB_GROUP * BROADCAST_MAX_SUB_GROUP];
    uint8_t bis_index_list[BROADCAST_MAX_BIS_PER_SUB_GROUP * BROADCAST_MAX_SUB_GROUP];
    wiced_bool_t b_encryption;
    wiced_bt_bap_broadcast_code_t broadcast_code;

    // profile info
    le_audio_bap_broadcast_base_t base;
} broadcast_sink_cb_t;

typedef struct
{
    uint32_t broadcast_id;
    wiced_bt_bap_broadcast_code_t broadcast_code;
} broadcast_source_t;

void broadcast_sink_bis_menu_discover_sources(bool start);
void broadcast_sink_bis_menu_sync_to_source(uint32_t broadcast_id, bool listen, uint8_t* broadcast_code);
void broadcast_sink_bis_init(wiced_bt_cfg_isoc_t *p_isoc_cfg);
void broadcast_sink_bis_discover_sources(wiced_bt_ble_scan_type_t scan_type);
void broadcast_sink_clear_data();
void broadcast_sink_bis_sync_to_source(wiced_bt_ble_scan_type_t scan_type, broadcast_source_t source);

broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_broadcast_id(uint32_t br_id);

broadcast_sink_cb_t *broadcast_sink_bis_alloc_big(uint32_t broadcast_id,
                                                  wiced_bt_device_address_t bd_addr,
                                                  uint8_t adv_sid);

void broadcast_sink_bis_free_big(broadcast_sink_cb_t *p_big);

broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_sync_handle(wiced_ble_padv_sync_handle_t sync_handle);

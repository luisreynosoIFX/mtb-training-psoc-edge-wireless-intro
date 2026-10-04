/******************************************************************************
* File Name        : broadcast_sink_bass.h
*
* Description      : This file is the public interface of broadcast_sink_bass.c.
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

#include "wiced_bt_types.h"

#include "broadcast_sink_bis.h"
#include "wiced_bt_ga_bass.h"

#define MAX_BASS 2

typedef struct
{
    wiced_bool_t is_used;
    uint16_t conn_id;
    wiced_bool_t waiting_broadcast_code;
    uint16_t sync_handle;
    wiced_bt_ga_bass_receive_state_t recv_state_data;
    wiced_bt_ga_bass_sub_group_data_t sub_group_data[WICED_BT_GA_BASS_MAX_SUBGROUP_COUNT]; /**< Subgroup Data */
} broadcast_sink_bass_data_t;

typedef struct
{
    broadcast_sink_bass_data_t bass_data[MAX_BASS];
    wiced_bt_ga_bass_operation_t operation_data;
} broadcast_sink_bass_t;

wiced_result_t broadcast_sink_bass_callback(uint16_t conn_id,
                                            void *p_app_ctx,
                                            const gatt_intf_service_object_t *p_service,
                                            wiced_bt_gatt_status_t status,
                                            uint32_t evt_type,
                                            gatt_intf_attribute_t *p_char,
                                            void *p_data,
                                            int len);

void broadcast_sink_bass_notify_pa_sync_state(wiced_ble_padv_sync_established_event_data_t *p_sync);
void broadcast_sink_bass_broadcast_code_check(uint16_t sync_handle);
void broadcast_sink_sync_to_source(broadcast_sink_cb_t *p_big, uint8_t *p_brdcst_code);
void broadcast_sink_bass_notify_sync_established(uint8_t *p_addr);
void broadcast_sink_bass_notify_big_sync_lost(uint8_t *p_addr);
void broadcast_sink_bass_notify_pa_sync_lost(uint16_t sync_handle);
broadcast_sink_bass_t *broadcast_sink_bass_get_bass_data();
void broadcast_sink_bass_request_pa_sync_info();

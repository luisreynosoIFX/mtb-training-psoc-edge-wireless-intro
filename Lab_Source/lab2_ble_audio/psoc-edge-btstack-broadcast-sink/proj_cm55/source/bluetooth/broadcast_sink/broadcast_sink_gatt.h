/******************************************************************************
* File Name        : broadcast_sink_gatt.h
*
* Description      : This file is the public interface of broadcast_sink_gatt.c.
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
#include "broadcast_sink_bass.h"
#include "wiced_bt_types.h"

/* App Library includes */
#include "wiced_bt_ga_common.h"

typedef struct
{
    gatt_intf_service_object_t *p_pacs;
    gatt_intf_service_object_t *p_bass;
} profiles_t;

typedef struct
{
    uint8_t in_use;
    uint16_t conn_id;
    uint32_t addr_type;
    wiced_bt_device_address_t bda;
    wiced_bool_t b_is_central;
} broadcast_sink_gatt_cb_t;

wiced_bt_gatt_status_t broadcast_sink_gatt_init(int max_connections, int max_mtu, ga_cfg_t *p_ga_cfg);
broadcast_sink_gatt_cb_t *broadcast_sink_gatt_get_cb_by_conn_id(uint16_t conn_id);
gatt_intf_service_object_t *broadcast_sink_gatt_get_bass_service_instance();

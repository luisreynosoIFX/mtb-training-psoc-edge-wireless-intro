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

#ifndef __WICED_BT_GA_HAS_H__
#define __WICED_BT_GA_HAS_H__


#include "wiced_bt_ble.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ga.h"
#include "wiced_bt_ga_common.h"

#define HAS_TRACE(...)
#define HAS_TRACE_CRIT(...)
#define HAS_TRACE_ARRAY(...)

#define HAS_MAX_PRESET_RECORD_NAME_LENGTH 40
#define HAS_BINAURAL_HEARING_AID 0
#define HAS_MONAURAL_HEARING_AID 1
#define HAS_BANDED_HEARING_AID (1 << 1)
#define HAS_PRESET_SYNCHRONIZATION_NOT_SUPPORTED (0 << 2)
#define HAS_PRESET_SYNCHRONIZATION_SUPPORTED (1 << 2)
#define HAS_IDENTICAL_PRESET_IN_COORDINATED_SET (0 << 3)
#define HAS_INDEPENDENT_PRESET_IN_COORDINATED_SET (1 << 3)
#define HAS_STATIC_PRESET (0 << 4)
#define HAS_DYNAMIC_PRESET (1 << 4)
#define HAS_WRITABLE_PRESETS_NOT_SUPPORTED (0 << 5)
#define HAS_WRITABLE_PRESETS_SUPPORTED (1<< 5)


    typedef struct
    {
        uint8_t preset_index;
        uint8_t properties;
        uint8_t isLast;
        wiced_bt_ga_string_t name;
    } wiced_bt_ga_has_preset_records_t;

    enum wiced_bt_ga_has_opcode_e
    {
        WICED_BT_GA_HAS_OPCODE_INVALID,
        WICED_BT_GA_HAS_OPCODE_READ_PRESETS_REQUEST,
        WICED_BT_GA_HAS_OPCODE_READ_PRESET_RESPONSE,
        WICED_BT_GA_HAS_OPCODE_PRESET_CHANGED,
        WICED_BT_GA_HAS_OPCODE_WRITE_PRESET_NAME,
        WICED_BT_GA_HAS_OPCODE_SET_ACTIVE_PRESET,
        WICED_BT_GA_HAS_OPCODE_SET_NEXT_PRESET,
        WICED_BT_GA_HAS_OPCODE_SET_PREVIOUS_PRESET,
        WICED_BT_GA_HAS_OPCODE_SET_ACTIVE_PRESET_SYNCHRONIZED_LOCALLY,
        WICED_BT_GA_HAS_OPCODE_SET_NEXT_PRESET_SYNCHRONIZED_LOCALLY,
        WICED_BT_GA_HAS_OPCODE_SET_PREVIOUS_PRESET_SYNCHRONIZED_LOCALLY
    };

    typedef uint8_t wiced_bt_ga_has_opcode_t;

    enum wiced_bt_ga_has_preset_change_id_e
    {
        WICED_BT_GA_HAS_GENERIC_UPDATE,
        WICED_BT_GA_HAS_PRESET_RECORD_DELETED,
        WICED_BT_GA_HAS_PRESET_RECORD_AVAILABLE,
        WICED_BT_GA_HAS_PRESET_RECORD_UNAVAILABLE
    } ;

    typedef uint8_t wiced_bt_ga_has_preset_change_id_t;

    typedef struct
    {
        uint8_t start_index;
        uint8_t num_presets;
    } wiced_bt_ga_has_cp_read_preset_t;

    typedef struct
    {
        wiced_bt_ga_has_preset_change_id_t change_id;
        uint8_t isLast;
        wiced_bt_ga_has_preset_records_t preset_rec;
        uint8_t prevIndex;
        uint8_t preset_index;
    } wiced_bt_ga_has_cp_rsp_preset_changed_t;

    typedef struct
    {
        wiced_bt_ga_has_opcode_t opcode;
        union {
            wiced_bt_ga_has_cp_read_preset_t read_preset;
            uint8_t preset_index;
            wiced_bt_ga_has_preset_records_t preset_rec;
        };
    } wiced_bt_ga_has_control_point_t;

    typedef struct
    {
        wiced_bt_ga_has_opcode_t opcode;
        union {
            uint8_t hearing_aid_feature;
            uint8_t active_preset_index;
            wiced_bt_ga_has_preset_records_t read_rsp_prest;
            wiced_bt_ga_has_cp_rsp_preset_changed_t preset_changed;
        };
    } wiced_bt_ga_has_evt_data_t;

    typedef union {
        wiced_bt_ga_has_evt_data_t evt_data;
        wiced_bt_ga_has_control_point_t has_cp_cmd;
    } wiced_bt_ga_has_data_t;

    wiced_result_t wiced_bt_ga_has_init(ga_cfg_t *p_cfg);

#endif //__WICED_BT_GA_HAS_H__

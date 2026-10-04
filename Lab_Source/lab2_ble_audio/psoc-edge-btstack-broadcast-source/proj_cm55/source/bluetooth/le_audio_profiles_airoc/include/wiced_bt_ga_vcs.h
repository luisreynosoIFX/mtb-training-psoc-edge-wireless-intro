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

/** @file
 *
 * Volume Control Service (VCS) Application Programming Interface
 */
#ifndef __WICED_BT_GA_VCS_H__
#define __WICED_BT_GA_VCS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "gatt_interface.h"
#include "wiced_bt_ga_common.h"
#include "wiced_bt_ga_aics.h"
#include "wiced_bt_ga_vocs.h"

#define VCS_TRACE(...)
#define VCS_TRACE_CRIT(...)

/**
 * @addtogroup Volume_And_Gain_Control_APIs
 * @{
 * @brief This profile enables a device to expose the controls and state of a device that can control the volume of an audio output such as one or more speakers and Control the peer device audio output as a client.
 * VCS may include zero or more instances of VOCS, AICS.
 * VCP which is the client side of the volume Control Service consists of APIs to control volume and gain of a peer device. It acts as client for VCS, VOCS and AICS
 */

/**
 * @addtogroup wiced_bt_ga_vcs
 * @{
 */

/** VCS minimum volume */
#define WICED_BT_GA_VCS_MINIMUM_VOLUME     0x0

/** VCS maximum volume */
#define WICED_BT_GA_VCS_MAXIMUM_VOLUME     0xFF

/** VCS muted state */
#define WICED_BT_GA_VCS_MUTED              0x1

/** VCS unmuted state */
#define WICED_BT_GA_VCS_NOT_MUTED          0x0

/** Volume profile opcode codes */
enum volume_control_opcodes_e
{
    VOLUME_CONTROL_OPCODE_RELATIVE_VOLUME_DOWN = 0x00,          /**< Relative volume down */
    VOLUME_CONTROL_OPCODE_RELATIVE_VOLUME_UP = 0x01,            /**< Relative volume up */
    VOLUME_CONTROL_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN = 0x02,   /**< Unmute and relative volume down */
    VOLUME_CONTROL_OPCODE_UNMUTE_RELATIVE_VOLUME_UP = 0x03,     /**< Unmute and relative volume up */
    VOLUME_CONTROL_OPCODE_SET_ABSOLUTE_VOLUME = 0x04,           /**< Set Absolute volume */
    VOLUME_CONTROL_OPCODE_UNMUTE = 0x05,                        /**< Unmute */
    VOLUME_CONTROL_OPCODE_MUTE = 0x06                           /**< Mute */
};

typedef uint8_t volume_control_opcodes_t; /**< VCS opcodes (see #volume_control_opcodes_e) */

/** Volume state data */
typedef struct
{
    uint8_t volume_setting;                        /**< current volume setting value */
    wiced_bt_ga_mute_val_t mute_state;             /**< current mute state value */
} wiced_bt_ga_vcs_volume_state_t;

/** Volume state control point data */
typedef struct
{
    volume_control_opcodes_t opcode;               /**< opcode of volume control point*/
    wiced_bt_ga_vcs_volume_state_t volume_state;   /**< volume state updated by the control point operation */
} wiced_bt_ga_vcs_control_point_t;

/** Volume service included service data */
typedef struct {
    uint8_t index;                           /**< index of the included service viz, vocs or aics */
    union {
        wiced_bt_ga_vocs_data_t* p_vocs;        /**< vocs data */
        wiced_bt_ga_aics_data_t* p_aics;        /**< aics data */
    };
} vcs_included_t;

/** Volume service data */
typedef union {
    wiced_bt_ga_vcs_control_point_t control_point_data;      /**< volume information */
    wiced_bt_ga_volume_flag_val_t   volume_flag;      /**< volume persistence flag */
    vcs_included_t                  volume_included;  /**< volume included service data */
} wiced_bt_ga_vcs_data_t;

/**
 * @brief Initialize the VCS service/profile
 *
 * @param[in] p_cfg : Generic Audio configuration
 */
wiced_result_t wiced_bt_ga_vcs_init(ga_cfg_t *p_cfg);

/**@} wiced_bt_ga_vcs */
#ifdef __cplusplus
}
#endif
/**@} Volume_And_Gain_Control_APIs */

#endif //__WICED_BT_GA_VCS_H__

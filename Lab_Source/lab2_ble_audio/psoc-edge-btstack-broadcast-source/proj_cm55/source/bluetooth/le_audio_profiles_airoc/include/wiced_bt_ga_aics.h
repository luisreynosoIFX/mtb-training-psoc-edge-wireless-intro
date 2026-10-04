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
 * Audio Input Control Service (AICS) Application Programming Interface
 */

#ifndef __WICED_BT_GA_AICS_H__
#define __WICED_BT_GA_AICS_H__

#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "gatt_interface.h"
#include "wiced_bt_ga_common.h"
/**
 * @addtogroup Volume_And_Gain_Control_APIs
 * @{
 */

/**
 * @addtogroup wiced_bt_ga_aics
 * @{
 * @brief This profile exposes the settings of an audio input such as a Bluetooth audio stream, microphone, etc. Multiple audio inputs may be combined as part of the server’s audio mixing functionality.
 */

/** AICS Structure */
typedef struct aics_t_ wiced_bt_ga_aics_t;

/** Audio Input Control Service opcodes */
typedef enum {
    WICED_BT_GA_AICS_OPCODE_SET_GAIN_SETTINGS      = 1,    /**< Set gain setting */
    WICED_BT_GA_AICS_OPCODE_SET_UNMUTE             = 2,    /**< Set mute state to unmute */
    WICED_BT_GA_AICS_OPCODE_SET_MUTE               = 3,    /**< Set mute state to mute */
    WICED_BT_GA_AICS_OPCODE_SET_MANUAL_GAIN_MODE   = 4,    /**< Set gain mode to manual */
    WICED_BT_GA_AICS_OPCODE_SET_AUTO_GAIN_MODE     = 5,    /**< Set gain mode to auto */
} wiced_bt_ga_aics_opcode_t;

/** AICS Mute mode */
typedef enum
{
    WICED_BT_GA_AICS_UNMUTE,                           /**< UnMuted */
    WICED_BT_GA_AICS_MUTE,                             /**< Muted */
    WICED_BT_GA_AICS_MUTE_DISABLED,                    /**< Mute Disabled */
} wiced_bt_ga_aics_mute_t;

/** AICS Gain Mode */
typedef enum
{
    WICED_BT_GA_AICS_GAIN_MODE_MANUAL_ONLY,    /**< Manual only */
    WICED_BT_GA_AICS_GAIN_MODE_AUTO_ONLY,      /**< Automatic only*/
    WICED_BT_GA_AICS_GAIN_MODE_MANUAL,         /**< Manual Gain Mode */
    WICED_BT_GA_AICS_GAIN_MODE_AUTO,           /**< Auto Gain Mode */
} wiced_bt_ga_aics_gain_mode_t;

/** AICS Input Type */
typedef enum
{
    WICED_BT_GA_AICS_INPUT_TYPE_UNSPECIFIED,       /**< Unspecified input */
    WICED_BT_GA_AICS_INPUT_TYPE_BLUETOOTH,         /**< Bluetooth audio stream */
    WICED_BT_GA_AICS_INPUT_TYPE_MICROPHONE,        /**< Microphone */
    WICED_BT_GA_AICS_INPUT_TYPE_ANALOG,            /**< Analog interface */
    WICED_BT_GA_AICS_INPUT_TYPE_DIGITAL,           /**< Digital interface */
    WICED_BT_GA_AICS_INPUT_TYPE_RADIO,             /**< AM / FM / XM / etc */
    WICED_BT_GA_AICS_INPUT_TYPE_STREAMING,         /**< Streaming audio source */
} wiced_bt_ga_aics_input_type_t;

/** AICS Input Status */
typedef enum
{
    WICED_BT_GA_AICS_INPUT_STATUS_INACTIVE,                       /**< Input Status Inactive */
    WICED_BT_GA_AICS_INPUT_STATUS_ACTIVE,                         /**< Input Status Active */
} wiced_bt_ga_aics_input_status_t;

/** AICS Gain Setting Attribute */
typedef struct {
    uint8_t   gain_setting_units;       /**< Gain Settings Units : 1 Unit = 0.1db */
    int8_t    max_gain_setting;         /**< Maximum Gain Setting */
    int8_t    min_gain_setting;         /**< Minimum Gain Setting */
} wiced_bt_ga_aics_gain_settings_params_t;

/** AICS Input State */
typedef struct {
    int8_t                         gain_setting;       /**< Gain Settings */
    wiced_bt_ga_aics_mute_t        mute_mode;          /**< Mute Mode */
    wiced_bt_ga_aics_gain_mode_t   gain_mode;          /**< Gain Mode */
} wiced_bt_ga_aics_input_state_t;

/** AICS Control Point data */
typedef struct {
    wiced_bt_ga_aics_opcode_t  opcode;                 /**< AICS control point opcode #wiced_bt_ga_aics_opcode_t */
    wiced_bt_ga_aics_input_state_t input_state;        /**< AICS Input State */
} wiced_bt_ga_aics_control_point_t;

/** @brief AICS data */
typedef union {
    wiced_bt_ga_aics_input_state_t          input_state;            /**< Audio Input State */
    wiced_bt_ga_aics_gain_settings_params_t gain_setting;           /**< Audio Input Gain setting */
    uint8_t                                 input_type;             /**< Audio Input Type */
    wiced_bt_ga_aics_input_status_t         input_status;           /**< Audio Input Status */
    wiced_bt_ga_string_t                    description;            /**< Audio Input descritpion */
    wiced_bt_ga_aics_control_point_t        control_point;          /**< Audio control point data */
} wiced_bt_ga_aics_data_t;

/**
 * \brief Notify AICS Input State change event to all the subscribed clients
 * \details On any change in the Input State, Application is expected to send notify Input State change api with all the connection ids of the clients.
 * AICS library sends notification to all subscribed clients
 *
 * @param[in] conn_id : GATT Connection ID
 * @param[in] p_service : AICS service object
 * @param[in] p_char : characteristic to be notified
 * @param[in] p_data : characteristic data to be notified 
 */
wiced_bt_gatt_status_t aics_notify(uint16_t conn_id,
                                   gatt_intf_service_object_t *p_service,
                                   gatt_intf_attribute_t *p_char,
                                   void *p_data);

/**
 * @brief AICS API to write characteristic data
 *
 * @param[in] conn_id : GATT Connection ID
 * @param[in] p_service : AICS service object
 * @param[in] p_char : characteristic to be written
 * @param[in] p_data : characteristic data to be written 
 */
wiced_bt_gatt_status_t aics_write_remote_attribute(uint16_t conn_id,
                                                 gatt_intf_service_object_t *p_service,
                                                 gatt_intf_attribute_t *p_char,
                                                 void *p_data);


#define AICS_TRACE(...) /**< Enable this to get AICS Trace */
#define AICS_TRACE_CRIT(...) /**< Enable this to get AICS Trace */
/**@} wiced_bt_ga_aics */
/**@} Volume_And_Gain_Control_APIs */

#endif //__WICED_BT_GA_AICS_H__

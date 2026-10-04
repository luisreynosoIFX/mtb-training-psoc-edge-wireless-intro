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
 * Volume Offset Control Service (VOCS) Application Programming Interface
 */

#ifndef __WICED_BT_GA_VOCS_H__
#define __WICED_BT_GA_VOCS_H__

#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "gatt_interface.h"
#include "wiced_bt_ga_common.h"

#define VOCS_TRACE(...)
#define VOCS_TRACE_CRIT(...)
/**
 * @addtogroup Volume_And_Gain_Control_APIs
 * @{
 */

/**
 * @addtogroup wiced_bt_ga_vocs
 * @{
 * @brief Exposes the offset level and location of an audio output such as a speaker.
 */
 
/** Volume Offset Control Service opcodes */
typedef enum {
    WICED_BT_GA_VOCS_OPCODE_SET_VOLUME_OFFSET = 1 /**< Set Volume Offset*/
} wiced_bt_ga_vocs_opcode_t;

/** Volume offset control point data */
typedef struct {
    wiced_bt_ga_vocs_opcode_t opcode; /**< VOCS opcode */
    int16_t volume_offset;            /**< VOCS offset value */
} vocs_opcode_t;

/** Volume offset data which is passed between application and profile */
typedef union {
    int16_t volume_offset;             /**< VOCS offset value */
    uint32_t audio_location;           /**< Audio location value */
    wiced_bt_ga_string_t description;  /**< VOCS offset value */
    vocs_opcode_t control_point;       /**< VOCS control point data */
}wiced_bt_ga_vocs_data_t;

/** Volume offset related methods defined in the init file*/
extern gatt_intf_service_methods_t vocs_methods;

/**
 * @brief VOCS API to notify characteristic data
 *
 * @param[in] conn_id : GATT Connection ID
 * @param[in] p_service : VOCS service object
 * @param[in] p_char : characteristic to be notified
 * @param[in] p_n : characteristic data to be notified 
 */
wiced_bt_gatt_status_t vocs_notify(uint16_t conn_id, gatt_intf_service_object_t* p_service, 
    gatt_intf_attribute_t *p_char, void *p_n);

/**
 * @brief VOCS API to write characteristic data
 *
 * @param[in] conn_id : GATT Connection ID
 * @param[in] p_service : VOCS service object
 * @param[in] p_char : characteristic to be written
 * @param[in] p_n : characteristic data to be written 
 */
wiced_bt_gatt_status_t vocs_write_remote_attribute(uint16_t conn_id, gatt_intf_service_object_t* p_service,
    gatt_intf_attribute_t *p_char, void* p_n);
	
	
/**@} wiced_bt_ga_vocs */
/**@} Volume_And_Gain_Control_APIs */

#endif //__WICED_BT_GA_VOCS_H__

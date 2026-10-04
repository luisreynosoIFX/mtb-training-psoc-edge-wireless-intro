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

#ifndef __WICED_BT_GA_MICS_H__
#define __WICED_BT_GA_MICS_H__

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

/**
 * @addtogroup Volume_And_Gain_Control_APIs
 * @{
 */

/**
 * @addtogroup wiced_bt_ga_mics
 * @{
 * @brief MICS is declared on devices that can control the mute state of a microphone’s audio. Only one instance of MICS is allowed.
 */

#define MICS_TRACE(...) /**< Enable this for MICS traces */
#define MICS_TRACE_CRIT(...) /**< Enable this for MICS library traces */

#define WICED_BT_MAX_AICS_INSTANCE          2 /**< Maximum Instance of AICS service supported by client */

/**
* @brief Included structure of MICS
*/
typedef struct {
    uint8_t index;                           /**< index of the included service viz, vocs or aics */
    union {
        wiced_bt_ga_aics_data_t* p_aics;        /**< aics data */
    };
} mics_included_t;

/**
* @brief MICS data structure
*/
typedef union {
    wiced_bt_ga_mute_val_t           mute_val;        /**< mute state */
    mics_included_t                  mics_included;   /**< volume included service data */
} wiced_bt_ga_mics_data_t;


/**
* @brief microphone State data
*/
typedef struct
{
	wiced_bt_ga_mute_val_t	mute_state; 					   /**< current mute state value of the peer*/
} wiced_bt_ga_microphone_state_data_t;

/**
* @brief microphone Control client status Data
*/
typedef union
{
	wiced_bt_ga_mute_val_t mute_state;					   /**< changed microphone setting value */
	uint8_t error_status;								   /**< error opcode in case of error event */
	wiced_bt_ga_aics_data_t aics_data;			           /**< AICS Status data */
} wiced_bt_ga_microphone_control_client_status_data_t;


/**
 * @brief Initialize the MICS service/profile
 *
 * @param[in] p_cfg : Generic Audio configuration
 * @result result of the init operation
 */
wiced_result_t wiced_bt_ga_mics_init(ga_cfg_t *p_cfg);

#ifdef __cplusplus
}
#endif
/**@} wiced_bt_ga_mics */
/**@} Volume_And_Gain_Control_APIs */


#endif /* __WICED_BT_GA_MICS_H__ */

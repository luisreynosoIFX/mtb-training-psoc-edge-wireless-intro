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
 * Co-ordinate Set Identification Service (CSIS) Application Programming Interface
 */

#ifndef __WICED_BT_GA_CSIS_H__
#define __WICED_BT_GA_CSIS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "gatt_interface.h"
#include "wiced_bt_ga_common.h"
#include "wiced_bt_ga_csis_common.h"

#define CSIS_TRACE(...)
#define CSIS_TRACE_CRIT(...)
#define CSIS_TRACE_ARRAY(...)

/**
 * @addtogroup Coordinate_Set_APIs
 * @{
 * @brief The Coordinated Set Identification Service (CSIS) can be used by devices to be discovered as part of a Coordinated Set. A Coordinated Set is defined as a group of devices that are configured to support a specific scenario.
 - Examples of Coordinated Sets include a pair of hearing aids, a pair of earbuds, or a speaker set that receives multi-channel audio and that reacts to control commands in a coordinated way (e.g., volume up and volume down).
 - CSIS is agnostic to the actual features and functions of the devices. The purpose of CSIS is to specify how a device can be discovered as part of a Coordinated Set and how to grant a client exclusive access to the Coordinated Set to avoid race conditions when multiple clients want to access the Coordinated Set at the same time.
 */

/**
 * @addtogroup wiced_bt_ga_csis
 * @{
 */

/**
 * \brief Generate PSRI valiue using the SIRK of the coordinated set.
 * \details PSRI value will be used during advertisement procedure to inform the clients regarding the specific set.
 *
 * @param[in]   sirk           SIRK value to be used for the specific coordinated set
 * @return  PSRI           PSRI value generated
 */
PSRI* wiced_bt_ga_csis_generate_psri(SIRK* sirk);

/**
 * \brief Set PSRI adv data.
 * \details Application use this api to advertise the set members coordinate set.
 *
 * @param[in]   psri           psri value to be set in the advertisement
 * @param[in]   adv_elem       memory in which the advertisement data has to be filled
 */
void wiced_bt_ga_csis_get_adv_data(PSRI* psri, wiced_bt_ble_advert_elem_t* adv_elem);

/**
 * \brief Set Lock timeout in seconds
 * \details Higher layer profile can set timeout value in seconds, it is by default set to 60 seconds.
 *
 * @param[in]   p_service    instance of the coordinated set identification service
 * @param[in]   timeout_in_sec timeout  value in seconds
 * @return  WICED_TRUE is operation is successful otherwise WICED_FALSE.
 */
void wiced_bt_ga_csis_set_lock_timeout_value(gatt_intf_service_object_t* p_service, uint8_t timeout_in_sec);

/**
 * \brief wiced_bt_ga_csis_is_operation_allowed
 * \details Other profile should invoke this API to check whether operation is allowed or not for given connection id.
 *
  * @param[in]   conn_id Connection id
 * @return  WICED_TRUE is operation is allowed otherwise WICED_FALSE.
 */
wiced_bool_t wiced_bt_ga_csis_is_operation_allowed(uint16_t conn_id);

/**
* Initialize the CSIS service_type/profile
* @param[in] p_cfg : Generic Audio configuration
*/
wiced_result_t wiced_bt_ga_csis_init(ga_cfg_t* p_cfg);

/**@} wiced_bt_ga_csis */
/**@} Coordinate_Set_APIs */
#ifdef __cplusplus
}
#endif

#endif //__WICED_BT_GA_CSIS_H__

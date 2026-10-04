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
 * Co-ordinate Set Identification Profile (CSIP) Application Programming Interface
 */

#ifndef __WICED_BT_GA_CSIP_H__
#define __WICED_BT_GA_CSIP_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ga_csis_common.h"

#define CSIP_TRACE(...)
#define CSIP_TRACE_CRIT(...)
#define CSIP_TRACE_ARRAY(...)

/**
 * @addtogroup Coordinate_Set_APIs
 * @{
 */

/**
 * @addtogroup wiced_bt_ga_csip
 * @{
 */


#define WICED_BT_GA_CSIP_SET_MEMBER_DISCOVERY_TIMEOUT_IN_SEC      10 /**< CSIP timeout to discover the set members set at 10 seconds */

/** Ordered Access complete callback to be set by the application, this will be invoked once the ordered access procedure is complete */
    typedef wiced_bt_gatt_status_t (*wiced_bt_ordered_access_procedure_cmpl_cb)(gatt_intf_service_object_t *p_csip_inst,
                                                                                void *data,
                                                                                wiced_bt_gatt_status_t status);
    /** Lock procedure complete callback to be set by the application, this will be invoked once the lock procedure is complete */
typedef wiced_bt_gatt_status_t (*wiced_bt_lock_procedure_cmpl_cb)(gatt_intf_service_object_t *p_csip_inst,
                                                                  void *data,
                                                                  wiced_bt_gatt_status_t status);

/** CSIP set entry per set memeber */
typedef struct {
    uint16_t conn_id; /**< GATT Connection ID */
    gatt_intf_service_object_t* p_service; /**< instance of the coordinated set identification client */
} wiced_bt_ga_csip_set_entry;

/**
* \brief Checks if the device belong to the co-ordinated set
* \details Checks if PSRI belongs to the co-ordinated set
*
* @param[in]   p_adv_report:  adv_report of the scan
* @param[in]   p_adv_data  :  adv data of the scan
* @param[in]   p_sirk      :  sirk value of the co-ordinate set to be found
*
* @return  wiced_bool_t  WICED_TRUE if psri belongs to coordinated set of sirk
*/
wiced_bool_t wiced_bt_ga_csip_check_if_belongs_to_coordinated_set(wiced_ble_ext_scan_results_t *p_adv_report,
                                                                  uint16_t adv_len,
                                                                  uint8_t *p_adv_data,
                                                                  SIRK *p_sirk);

/**
* \brief Read set identification resolving key
* \details Reads the set identification resolving key characteristic of the csis
*
* @param[in] conn_id : GATT Connection ID
* @param[in] p_service :  instance of the coordinated set identification client
*
* @return  wiced_bt_gatt_status_t result of the read operation
*/
wiced_bt_gatt_status_t wiced_bt_ga_csip_read_set_identity_resolving_key(uint16_t conn_id, gatt_intf_service_object_t * p_service);

/**
* \brief Read lock state
* \details Reads the lock state of the csis instance
*
* @param[in] conn_id : GATT Connection ID
* @param[in] p_service :  instance of the coordinated set identification client
*
* @return  wiced_bt_gatt_status_t result of the read operation
*/
wiced_bt_gatt_status_t wiced_bt_ga_csip_read_lock_value(uint16_t conn_id, gatt_intf_service_object_t * p_service);

/**
* \brief Set lock state
* \details Sets the lock state of the csis instance
*
* @param[in]   conn_id : GATT Connection ID
* @param[in]   p_service :  instance of the coordinated set identification client
* @param[in]   lock_val  :    lock value to be set
*
* @return  wiced_bt_gatt_status_t result of the read operation
*/
wiced_bt_gatt_status_t wiced_bt_ga_csip_request_lock(uint16_t conn_id, gatt_intf_service_object_t * p_service, wiced_bt_ga_csis_lock_val_t lock_val);

/**
* \brief Lock Procedure
* \details Tries to aquire lock from the least rank device to highest rank. If any device responds with lock denied unlocks already locked devices.
*
*
* @param[in]   p_set : csip set memeber entries to perform lock procedure
* @param[in]   num_instances : number of client instances sent
* @param[in]   lock_val : lock value to be set
* @param[in]   cb : callback returned when the complete lock procedure is completed
* @param[in]   data pointer : this will be return in given cb
*/
wiced_bt_gatt_status_t wiced_bt_ga_csip_start_lock_procedure(wiced_bt_ga_csip_set_entry *p_set,
                                                             int num_instances,
                                                             wiced_bt_ga_csis_lock_val_t lock_val,
                                                             wiced_bt_lock_procedure_cmpl_cb cb,
                                                             void *data);

/**
* \brief Ordered Access Procedure
* \details Reads lock value from the least rank device to highest rank. If any device responds with locked,
* returns with the event WICED_BT_GA_CSIP_ORDERED_ACCESS_PROCEDURE_COMPLETE with status set to the procedure status
*
* @param[in]   p_set : csip set memeber entries to be locked
* @param[in]   num_instances : number of client instances sent
* @param[in]   cb : callback returned when the complete ordered access procedure is completed
* @param[in]   data pointer : this swill be return in given cb
*/
void wiced_bt_ga_csip_start_ordered_access_procedure(wiced_bt_ga_csip_set_entry* p_set, int num_instances, wiced_bt_ordered_access_procedure_cmpl_cb cb, void *data);

/**
* Checks if lock procedure is in progress
*
* @return  wiced_bool_t  WICED_TRUE if lock procedure is in progress
*/
wiced_bool_t wiced_bt_ga_csip_is_lock_procedure_in_progress(void);

/**@} wiced_bt_ga_csip */
/**@} Coordinate_Set_APIs */

#ifdef __cplusplus
}
#endif

#endif //__WICED_BT_GA_CSIP_H__

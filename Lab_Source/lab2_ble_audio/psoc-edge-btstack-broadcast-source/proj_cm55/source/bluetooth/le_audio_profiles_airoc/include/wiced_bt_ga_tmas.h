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

#ifndef __WICED_BT_GA_TMAS_H__
#define __WICED_BT_GA_TMAS_H__

#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "gatt_interface.h"
#include "wiced_bt_ga_common.h"

#define TMAS_TRACE(...)
#define TMAS_TRACE_CRIT(...)

/** @brief TMAP Role */
typedef enum
{
    TMAP_ROLE_CALL_GATEWAY                  =  1,          /**< Call gateway */
    TMAP_ROLE_CALL_TERMINAL                 = (1 << 1),   /**< Call  terminal*/
    TMAP_ROLE_UNICAST_MEDIA_SENDER          = (1 << 2),   /**< Unicast media sender */
    TMAP_ROLE_UNICAST_MEDIA_RECEIVER        = (1 << 3),   /**< Unicast media receiver */
    TMAP_ROLE_BROADCAST_MEDIA_SENDER        = (1 << 4),   /**< Broadcast media sender */
    TMAP_ROLE_BROADCAST_MEDIA_RECEIVER      = (1 << 5),   /**< Broadcast media receiver */
} tmap_role_t;

/** @brief Audio Input Control Service event data */
typedef union
{
    tmap_role_t tmap_role; /**< Supported TMAP roles defined in tmap_role_t */

} wiced_bt_ga_tmas_data_t;


/**
 * @brief Initialize the TMAS service/profile
 *
 * @param[in] p_cfg : Generic Audio configuration
 * @result result of the init operation
 */

wiced_result_t wiced_bt_ga_tmas_init(ga_cfg_t *p_cfg);

#endif // __WICED_BT_GA_TMAS_H__

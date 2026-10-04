/******************************************************************************
* File Name        : broadcast_source_bt_manager.c
*
* Description      : Bluetooth management event handling for Broadcast Source.
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

/*******************************************************************************
* Header Files
*******************************************************************************/
/* Application includes */
#include "broadcast_source_bt_manager.h"
#include "broadcast_source.h"
#include "app_bt_utils.h"

/* BT Stack includes */
#include "wiced_bt_trace.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_gatt.h"
#include "logging.h"

/*******************************************************************************
* Function Name: broadcast_source_handle_btm_enabled
********************************************************************************
* Summary:
*   Set the local Bluetooth address and start the default broadcast.
*
* Parameters:
*   p_btm_enabled : Stack-enabled event data (unused).
*
* Return:
*   WICED_SUCCESS after requesting broadcast startup.
*******************************************************************************/
static wiced_result_t broadcast_source_handle_btm_enabled(wiced_bt_dev_enabled_t *p_btm_enabled)
{
    if (p_btm_enabled->status != WICED_BT_SUCCESS)
    {
        printf(">> Error: Bluetooth enable failed (0x%02x).\n", p_btm_enabled->status);
        return WICED_ERROR;
    }

    set_local_bd_addr();
    printf("Starting audio broadcast...\n");
    broadcast_source_init_and_start();

    return WICED_SUCCESS;
}

/*******************************************************************************
* Function Name: broadcast_source_btm_cback
********************************************************************************
* Summary:
*   Handle the stack-enabled event for the Broadcast Source application.
*
* Parameters:
*   event        : Bluetooth management event.
*   p_event_data : Event-specific data.
*
* Return:
*   Handler result for BTM_ENABLED_EVT; WICED_ERROR for other events.
*******************************************************************************/
wiced_result_t broadcast_source_btm_cback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t res = WICED_ERROR;

    WICED_BT_TRACE("[%s] Received Event [%d] \n", __FUNCTION__, event);

    switch (event)
    {
        case BTM_ENABLED_EVT:
            res = broadcast_source_handle_btm_enabled(&p_event_data->enabled);
            break;

        default:
            break;
    }

    return res;
}

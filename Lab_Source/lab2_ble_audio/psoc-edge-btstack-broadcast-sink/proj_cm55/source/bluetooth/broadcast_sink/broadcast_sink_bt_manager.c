/******************************************************************************
* File Name        : broadcast_sink_bt_manager.c
*
* Description      : Bluetooth management events and broadcast sink startup.
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

/******************************************************************************
* Header Files
*******************************************************************************/

#include "cybsp.h"

/* Application includes */
#include "broadcast_sink_bt_manager.h"
#include "broadcast_sink_bis.h"
#include "broadcast_sink_gatt.h"

/* BT Stack includes */
#include "wiced_bt_trace.h"
#include "wiced_bt_stack.h"
#include "broadcast_sink_bis.h"
#include "logging.h"
extern wiced_bt_cfg_ble_t broadcast_sink_ble_cfg;
extern wiced_bt_cfg_isoc_t broadcast_sink_isoc_cfg;
extern void set_local_bd_addr(void);


ga_cfg_t broadcast_sink_ga_cfg = {
    .pacs_max_sink_capabilities_supported = 2,
    .pacs_max_source_capabilities_supported = 0,

    .bass_max_receive_state_supported = 2,
};


wiced_result_t broadcast_sink_handle_btm_enabled(wiced_bt_dev_enabled_t *p_btm_enabled)
{
    wiced_bt_gatt_status_t sts = WICED_BT_ERROR;

    if (p_btm_enabled->status != WICED_BT_SUCCESS)
    {
        printf(">> Error: Bluetooth enable failed (0x%02x).\n", p_btm_enabled->status);
        return WICED_ERROR;
    }

    set_local_bd_addr();

    /* Initialize GATT */
    sts = broadcast_sink_gatt_init(broadcast_sink_ble_cfg.ble_max_simultaneous_links,
                                   broadcast_sink_ble_cfg.ble_max_rx_pdu_size,
                                   &broadcast_sink_ga_cfg);

    if (sts != WICED_BT_GATT_SUCCESS)
    {
        printf(">> Error: Broadcast Sink GATT initialization failed (0x%02x).\n", sts);
        return WICED_ERROR;
    }

    /* Initialize BIS */
    broadcast_sink_bis_init(&broadcast_sink_isoc_cfg);

    printf("Broadcast Sink initialized. Scanning for broadcast sources...\n");
    broadcast_sink_bis_menu_discover_sources(true);

    return WICED_SUCCESS;
}

wiced_result_t broadcast_sink_btm_cback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data)
{
     wiced_result_t res = WICED_ERROR;

    WICED_BT_TRACE("[%s] Received Event [%d] \n", __FUNCTION__, event);

    switch (event)
    {
        case BTM_ENABLED_EVT:
            res = broadcast_sink_handle_btm_enabled(&p_event_data->enabled);
            break;

        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT: {
        wiced_bt_dev_ble_io_caps_req_t *p_ble_io_caps = &p_event_data->pairing_io_capabilities_ble_request;
        p_ble_io_caps->local_io_cap = BTM_IO_CAPABILITIES_DISPLAY_AND_YES_NO_INPUT;
            p_ble_io_caps->oob_data = BTM_OOB_NONE;
            p_ble_io_caps->auth_req = BTM_LE_AUTH_REQ_SC_MITM_BOND;
            p_ble_io_caps->max_key_size = 16;
            p_ble_io_caps->init_keys = BTM_LE_KEY_PENC | BTM_LE_KEY_PID | BTM_LE_KEY_PCSRK | BTM_LE_KEY_LENC;
            p_ble_io_caps->resp_keys = BTM_LE_KEY_PENC | BTM_LE_KEY_PID | BTM_LE_KEY_PCSRK | BTM_LE_KEY_LENC;
        }
        break;

        case BTM_SECURITY_REQUEST_EVT:
            wiced_bt_ble_security_grant(p_event_data->security_request.bd_addr, WICED_BT_SUCCESS);
            break;

        case BTM_USER_CONFIRMATION_REQUEST_EVT:
            wiced_bt_dev_confirm_req_reply(WICED_BT_SUCCESS, p_event_data->user_confirmation_request.bd_addr);
            break;

        case BTM_PAIRING_COMPLETE_EVT:
            WICED_BT_TRACE("[%s] status %d\n",
                           __FUNCTION__,
                           p_event_data->pairing_complete.pairing_complete_info.ble.status);
            break;

        default:
            break;
    }

    return res;
}

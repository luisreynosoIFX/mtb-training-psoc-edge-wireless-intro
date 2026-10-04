/******************************************************************************
* File Name        : iso_data_handler.c
*
* Description      : This file contains the source code for ISO packet handling.
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "wiced_bt_isoc.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
#include "iso_data_handler.h"
#include "logging.h"

/******************************************************************************
* Macros
*******************************************************************************/
#define ISO_DATA_HEADER_SIZE 4

#define ISO_LOAD_HEADER_SIZE_WITH_TS 8
#define ISO_LOAD_HEADER_SIZE_WITHOUT_TS 4

#define ISO_PKT_PB_FLAG_MASK 3
#define ISO_PKT_PB_FLAG_OFFSET 12

#define ISO_PKT_PB_FLAG_FIRST_FRAGMENT 0
#define ISO_PKT_PB_FLAG_CONTINUATION_FRAGMENT 1
#define ISO_PKT_PB_FLAG_COMPLETE 2
#define ISO_PKT_PB_FLAG_LAST_FRAGMENT 3

#define ISO_PKT_TS_FLAG_MASK 1
#define ISO_PKT_TS_FLAG_OFFSET 14

#define ISO_PKT_RESERVED_FLAG_MASK 1
#define ISO_PKT_RESERVED_FLAG_OFFSET 15

#define ISO_PKT_DATA_LOAD_LENGTH_MASK 0x3FFF
#define ISO_PKT_SDU_LENGTH_MASK 0x0FFF

/******************************************************************************
* Global Variables
*******************************************************************************/
static iso_dhm_num_complete_evt_cb_t g_num_complete_cb;
static iso_dhm_rx_evt_cb_t g_rx_data_cb;


/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: iso_dhm_process_rx_data
*******************************************************************************
* Summary:
*   Process received HCI ISO data and pass the SDU to the registered callback.
*
* Parameters:
*   p_data : Pointer to the received HCI ISO packet
*   length : Received packet length in bytes
*
* Return:
*   None
*
*******************************************************************************/
void iso_dhm_process_rx_data(uint8_t *p_data, uint32_t length)
{
    uint16_t handle_and_flags = 0;
    uint16_t data_load_length = 0;
    uint16_t ts_flag = 0;
    uint16_t pb_flag = 0;
    uint16_t psn = 0;
    uint16_t sdu_len = 0;
    uint32_t ts = 0;

    if (!length) { return; }

    STREAM_TO_UINT16(handle_and_flags, p_data);
    STREAM_TO_UINT16(data_load_length, p_data);

    pb_flag = (handle_and_flags & (ISO_PKT_PB_FLAG_MASK << ISO_PKT_PB_FLAG_OFFSET))
               >> ISO_PKT_PB_FLAG_OFFSET;
    ts_flag = (handle_and_flags & (ISO_PKT_TS_FLAG_MASK << ISO_PKT_TS_FLAG_OFFSET))
               >> ISO_PKT_TS_FLAG_OFFSET;

    handle_and_flags &= ~(ISO_PKT_PB_FLAG_MASK << ISO_PKT_PB_FLAG_OFFSET);
    handle_and_flags &= ~(ISO_PKT_TS_FLAG_MASK << ISO_PKT_TS_FLAG_OFFSET);
    handle_and_flags &= ~(ISO_PKT_RESERVED_FLAG_MASK << ISO_PKT_RESERVED_FLAG_OFFSET);

    if (ts_flag) { STREAM_TO_UINT32(ts, p_data); }

    STREAM_TO_UINT16(psn, p_data);
    STREAM_TO_UINT16(sdu_len, p_data);

    data_load_length &= ISO_PKT_DATA_LOAD_LENGTH_MASK;
    sdu_len &= ISO_PKT_SDU_LENGTH_MASK;

    (void) data_load_length;


    (void)ts;
    (void)pb_flag;
    (void)psn;

    if ((sdu_len > 0) && g_rx_data_cb)
    {
        g_rx_data_cb(handle_and_flags, p_data, sdu_len);
    }

}

/******************************************************************************
* Function Name: iso_dhm_process_num_completed_pkts
*******************************************************************************
* Summary:
*   Process completed ISO packet counts and notify the registered callback.
*
* Parameters:
*   p_buf : Pointer to the completed-packet event parameters
*
* Return:
*   WICED_FALSE for a null buffer or an unrecognized handle; WICED_TRUE otherwise.
*******************************************************************************/
wiced_bool_t iso_dhm_process_num_completed_pkts(uint8_t *p_buf)
{
    if (p_buf == NULL)
    {
        WICED_BT_TRACE("Error: p_buf is NULL\n");
        return WICED_FALSE;
    }
    uint8_t num_handles, xx;
    uint16_t handle;
    uint16_t num_sent;
    wiced_bool_t complete = WICED_TRUE;

    STREAM_TO_UINT8(num_handles, p_buf);

    for (xx = 0; xx < num_handles; xx++)
    {
        STREAM_TO_UINT16(handle, p_buf);
        STREAM_TO_UINT16(num_sent, p_buf);


        //validate handle
        if (wiced_ble_isoc_is_cis_connected_with_conn_hdl(handle) ||
            wiced_ble_isoc_is_bis_created(handle))
        {
            //callback to app to send more packets
            if (g_num_complete_cb) { g_num_complete_cb(handle, num_sent); }
        }
        else
        {
            complete = WICED_FALSE;
        }
    }
    return complete;
}

/******************************************************************************
* Function Name: iso_dhm_init
*******************************************************************************
* Summary:
*   Initialize the ISO data handler and register its callbacks.
*
* Parameters:
*   num_complete_cb : Callback for completed ISO packet counts
*   rx_data_cb      : Callback for received ISO SDUs
*
* Return:
*   None
*
*******************************************************************************/
void iso_dhm_init(iso_dhm_num_complete_evt_cb_t num_complete_cb,
                  iso_dhm_rx_evt_cb_t rx_data_cb)
{
    wiced_ble_isoc_register_data_cb(iso_dhm_process_rx_data,
                                    iso_dhm_process_num_completed_pkts);

    g_num_complete_cb = num_complete_cb;
    g_rx_data_cb = rx_data_cb;
}

/******************************************************************************
* Function Name: iso_dhm_get_header_size
*******************************************************************************
* Summary:
*   Return the HCI ISO header size including a timestamp.
*
* Parameters:
*   None
*
* Return:
*   Header size in bytes.
*******************************************************************************/
uint32_t iso_dhm_get_header_size(void)
{
    return ISO_LOAD_HEADER_SIZE_WITH_TS + ISO_DATA_HEADER_SIZE;
}

/* [] END OF FILE */

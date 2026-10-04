/******************************************************************************
* File Name        : broadcast_sink_bis.c
*
* Description      : Broadcast source discovery and BIS synchronization.
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

/* Application includes */
#include "broadcast_sink_bis.h"
#include "broadcast_sink_bass.h"


#include "le_audio_pbp.h"
#include "broadcast_sink_iso_audio.h"
/* BT Stack includes */
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
#include "le_audio_pbp.h"

#include "logging.h"

#include "audio_driver_psoc.h"
/*******************************************************************************
* Macros
*******************************************************************************/
#define MAX_BIG 20








/******************************************************************************
* Global Variables
******************************************************************************/
broadcast_sink_cb_t g_broadcast_sink_cb[MAX_BIG] = {0};
static wiced_bool_t broadcast_sink_periodic_sync_in_progress = FALSE;
broadcast_source_t broadcast_source = {0};

void broadcast_sink_create_periodic_sync(wiced_ble_ext_scan_results_t *p_scan_result);

/******************************************************************************
* Function Prototypes
******************************************************************************/
void broadcast_sink_bis_menu_discover_sources(bool start);
void broadcast_sink_bis_menu_sync_to_source(uint32_t broadcast_id, bool listen, uint8_t* broadcast_code);
void broadcast_sink_bis_clear_data(void);
void broadcast_sink_bis_sync_to_source(wiced_bt_ble_scan_type_t scan_type, broadcast_source_t source);
void broadcast_sink_bis_discover_sources(wiced_bt_ble_scan_type_t scan_type);
void broadcast_sink_bis_free_big(broadcast_sink_cb_t *p_big);
void broadcast_sink_bis_init(wiced_bt_cfg_isoc_t *p_isoc_cfg);
broadcast_sink_cb_t *broadcast_sink_bis_alloc_big(uint32_t broadcast_id, wiced_bt_device_address_t bd_addr, uint8_t adv_sid);
broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_sync_handle(wiced_ble_padv_sync_handle_t sync_handle);
broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_broadcast_id(uint32_t br_id);

static void priv_periodic_adv_report_handler(wiced_ble_ext_adv_event_t event, wiced_ble_ext_adv_event_data_t *p_ed);
static void broadcast_sink_bis_menu_ext_adv_scan_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_data_len, uint8_t *p_adv_data);
static void broadcast_sink_bis_ext_adv_scan_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_data_len, uint8_t *p_adv_data);
static void broadcast_sink_bis_ext_adv_scan_to_sync_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_data_len, uint8_t *p_adv_data);
static void broadcast_sink_bis_isoc_cb(wiced_ble_isoc_event_t event, wiced_ble_isoc_event_data_t *p_ed);
static void broadcast_sink_bis_update_sync_handle(wiced_bt_device_address_t bd_addr, wiced_ble_ext_adv_sid_t adv_sid, wiced_ble_padv_sync_handle_t sync_handle);
static broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_adv_sid(wiced_bt_device_address_t bd_addr, wiced_ble_ext_adv_sid_t adv_sid);
static broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_handle(uint8_t big_handle);



#define MAX_BIG 20
#define DEFAULT_VOL 100   /* default volume 0-255 scale (~78%) */
/******************************************************************************
 * Function Name: broadcast_sink_bis_get_big_by_handle
 *******************************************************************************
 * Summary:
 *   Get big by handle
 *
 * Parameters:
 *   uint8_t big_handle :   big handle
 *
 * Return:
 *   broadcast_sink_cb_t : information structure
 *
 ******************************************************************************/
broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_handle(uint8_t big_handle)
{
    for (size_t i = 0; i < MAX_BIG; i++)
    {
        if (TRUE == g_broadcast_sink_cb[i].in_use && big_handle == g_broadcast_sink_cb[i].big_handle)
        {
            return &g_broadcast_sink_cb[i];
        }
    }

    return NULL;
}

/******************************************************************************
 * Function Name: broadcast_sink_bis_get_big_by_broadcast_id
 *******************************************************************************
 * Summary:
 *   Get big by broadcast ID
 *
 * Parameters:
 *   uint32_t br_id :   broadcast ID
 *
 * Return:
 *   broadcast_sink_cb_t : information structure
 *
 ******************************************************************************/
broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_broadcast_id(uint32_t br_id)
{
    for (size_t i = 0; i < MAX_BIG; i++)
    {
        if (TRUE == g_broadcast_sink_cb[i].in_use && br_id == g_broadcast_sink_cb[i].base.broadcast_id)
        {
            return &g_broadcast_sink_cb[i];
        }
    }

    return NULL;
}

broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_adv_sid(wiced_bt_device_address_t bd_addr,
                                                           wiced_ble_ext_adv_sid_t adv_sid)
{
    WICED_BT_TRACE("[%s] Searching for [addr:%B] [adv_sid:%d] \n", __FUNCTION__, bd_addr, adv_sid);

    for (size_t i = 0; i < MAX_BIG; i++)
    {

        if (TRUE == g_broadcast_sink_cb[i].in_use &&
            0 == memcmp(bd_addr, g_broadcast_sink_cb[i].bd_addr, BD_ADDR_LEN) &&
            adv_sid == g_broadcast_sink_cb[i].adv_handle)
        {
            WICED_BT_TRACE("[%s] Entry %d is matching\n", __FUNCTION__, i);
            return &g_broadcast_sink_cb[i];
        }
    }

    return NULL;
}

/******************************************************************************
 * Function Name: broadcast_sink_bis_menu_sync_to_source
 *******************************************************************************
 * Summary:
 *   Manually sync to source
 *
 * Parameters:
 *   uint32_t broadcast_id  : broadcast ID
 *   bool listen            : listen to source or not
 *   uint8_t *broadcast_code : Pointer to the 16-byte broadcast code
 *
 * Return:
 *
 ******************************************************************************/
void broadcast_sink_bis_menu_sync_to_source(uint32_t broadcast_id, bool listen, uint8_t* broadcast_code)
{
    broadcast_sink_cb_t *p_big = NULL;
    broadcast_source_t source = {0};

    uint8_t lc3_index[2] = {0};

    source.broadcast_id = broadcast_id;
    TRACE_LOG("Broadcast ID : 0x%lx", (unsigned long) source.broadcast_id);

    memcpy(&source.broadcast_code, broadcast_code, 16);
    WICED_BT_TRACE_ARRAY(&source.broadcast_code, 16, "Broadcast code :\n");

    if (!listen)
    {
        p_big = broadcast_sink_bis_get_big_by_broadcast_id(source.broadcast_id);
        if (p_big == NULL)
        {
            TRACE_ERR("[%s] p_big is null", __FUNCTION__);
            return;
        }

        wiced_ble_isoc_peripheral_big_terminate_sync(p_big->big_handle);
        wiced_ble_padv_terminate_sync(p_big->sync_handle);
        iso_audio_remove_data_path(p_big->bis_conn_id_list[0], WICED_BLE_ISOC_DPD_OUTPUT_BIT, lc3_index);
        broadcast_sink_bis_free_big(p_big);
    }

    broadcast_sink_bis_sync_to_source(listen ? BTM_BLE_SCAN_TYPE_HIGH_DUTY : BTM_BLE_SCAN_TYPE_NONE, source);
}

/******************************************************************************
 * Function Name: broadcast_sink_bis_get_big_by_sync_handle
 *******************************************************************************
 * Summary:
 *   Get big by sync handle
 *
 * Parameters:
 *   wiced_ble_padv_sync_handle_t sync_handle    : sync handle
 *
 * Return:
 *  broadcast_sink_cb_t : information structure
 *
 ******************************************************************************/
broadcast_sink_cb_t *broadcast_sink_bis_get_big_by_sync_handle(wiced_ble_padv_sync_handle_t sync_handle)
{
    for (size_t i = 0; i < MAX_BIG; i++)
    {
        if (TRUE == g_broadcast_sink_cb[i].in_use && sync_handle == g_broadcast_sink_cb[i].sync_handle)
        {
            return &g_broadcast_sink_cb[i];
        }
    }

    return NULL;
}

void broadcast_sink_bis_update_sync_handle(wiced_bt_device_address_t bd_addr,
                                           wiced_ble_ext_adv_sid_t adv_sid,
                                           wiced_ble_padv_sync_handle_t sync_handle)
{
    broadcast_sink_cb_t *p_big = broadcast_sink_bis_get_big_by_adv_sid(bd_addr, adv_sid);
    if (!p_big) return;

    WICED_BT_TRACE("[%s] Updating sync_handle to %d\n", __FUNCTION__, sync_handle);

    p_big->sync_handle = sync_handle;
}

void priv_periodic_adv_report_handler(wiced_ble_ext_adv_event_t event, wiced_ble_ext_adv_event_data_t *p_ed)
{
    broadcast_sink_cb_t *p_big = NULL;
    wiced_ble_padv_sync_handle_t sync_handle = (WICED_BLE_PERIODIC_ADV_REPORT_EVENT == event)
                                                              ? p_ed->periodic_adv_report.sync_handle
                                                              : p_ed->biginfo_adv_report.sync_handle;
        WICED_BT_TRACE("[PA] report event=%d sync_handle=%d\n", event, (int)sync_handle);
    wiced_ble_isoc_big_create_sync_t create_sync = {0};
    p_big = broadcast_sink_bis_get_big_by_sync_handle(sync_handle);
    if (!p_big)
    {
        WICED_BT_TRACE("[%s] p_big is NULL\n", __FUNCTION__, sync_handle);
        return;
    }

    // check if the report is processed already
    if (WICED_BLE_PERIODIC_ADV_REPORT_EVENT == event && !p_big->b_base_updated)
    {
        /* received truncated data, discard the report */
        if (2 == p_ed->periodic_adv_report.data_status)
        {
            WICED_BT_TRACE("[%s] Dropping truncated PA report\n", __FUNCTION__);
            if (p_big->p_periodic_adv_data)
            {
                wiced_bt_free_buffer(p_big->p_periodic_adv_data);
                p_big->p_periodic_adv_data = NULL;
            }
            return;
        }
WICED_BT_TRACE("[BASE] data_status=%d len=%d buf=%p\n",
       p_ed->periodic_adv_report.data_status,
       p_ed->periodic_adv_report.data_length,
       p_big->p_periodic_adv_data);
        /* Allocate memory to store the periodic adv data */
        if (!p_big->p_periodic_adv_data)
        {
            /* Discard if the report is not Basic Announcement  */
            if (!le_audio_bap_broadcast_is_basic_announcement(p_ed->periodic_adv_report.p_data, p_ed->periodic_adv_report.data_length, &p_big->base_len))
            {
                /* Fragment data (data_status != 0) is incomplete, so is_basic_announcement
                   always fails on it. Use a safe buffer size and continue accumulating. */
                if (p_ed->periodic_adv_report.data_status != 0)
                    p_big->base_len = 255;
                else
                    return;  /* complete packet, not a Basic Announcement — discard */
            }

            p_big->p_periodic_adv_data = wiced_bt_get_buffer(255);
            WICED_BT_TRACE("[%s] p_periodic_adv_data 0x%x [%d bytes]\n",
                           __FUNCTION__,
                           p_big->p_periodic_adv_data,
                           p_big->base_len);
            if (!p_big->p_periodic_adv_data) return;

            p_big->periodic_adv_data_offset = 0;
        }

        /* Store data for future if partial data is received */
        if (p_ed->periodic_adv_report.data_status)
        {
            memcpy(p_big->p_periodic_adv_data + p_big->periodic_adv_data_offset,
                   p_ed->periodic_adv_report.p_data,
                   p_ed->periodic_adv_report.data_length);

            p_big->periodic_adv_data_offset += p_ed->periodic_adv_report.data_length;
            WICED_BT_TRACE("[%s] cached partial PA report\n", __FUNCTION__);

            return;
        }

        WICED_BT_TRACE("[%s] Received a new PA report [SyncHandle:%d] len %d base len %d\n",
                       __FUNCTION__,
                       sync_handle,
                       p_ed->periodic_adv_report.data_length,
                       p_big->base_len);

        memcpy(p_big->p_periodic_adv_data + p_big->periodic_adv_data_offset,
               p_ed->periodic_adv_report.p_data,
               p_ed->periodic_adv_report.data_length);

        // Parse the received BASE after skipping the two-byte AD length/type header.
        // Use actual assembled data length, not base_len (which may be a 255-byte fallback for fragments)
le_audio_bap_broadcast_parse_base_info(p_big->p_periodic_adv_data + 2,
                                       (p_big->periodic_adv_data_offset + p_ed->periodic_adv_report.data_length) - 2,
                                       &p_big->base);
        wiced_bt_free_buffer(p_big->p_periodic_adv_data);
        p_big->p_periodic_adv_data = NULL;

        p_big->b_base_updated = TRUE;
        p_big->base.state = BAP_BROADCAST_STATE_CONFIGURED;
    }
    else if (WICED_BT_BLE_BIGINFO_ADV_REPORT_EVENT == event)
    {
        /* Discard if BASE is not parsed yet or BIGInfo is already updated*/
        if (!p_big->b_base_updated || p_big->b_biginfo_updated) return;

        /* Enable encryption if stream is encrypted */
        p_big->b_encryption = p_ed->biginfo_adv_report.encryption;
        p_big->b_biginfo_updated = TRUE;
        p_big->base.state = BAP_BROADCAST_STATE_STREAMING;

            create_sync.big_handle = p_big->big_handle;
            create_sync.sync_handle = p_big->sync_handle;
            create_sync.max_sub_events = p_big->number_of_subevents;
            create_sync.big_sync_timeout = 10;

            create_sync.num_bis = p_big->base.sub_group[0].bis_cnt;
            create_sync.bis_idx_list = p_big->bis_index_list;
            uint8_t idx = 0;
            for (uint8_t i = 0; i < p_big->base.sub_group_cnt; i++)
            {
                for (uint8_t j = 0; j < p_big->base.sub_group[i].bis_cnt; j++)
                {
                    create_sync.bis_idx_list[idx] =
                        p_big->base.sub_group[i].bis_config[j].bis_idx;
                    WICED_BT_TRACE("[%s] bis idx %d", __FUNCTION__, create_sync.bis_idx_list[idx]);
                    idx++;
                }
            }
            create_sync.encrypt = p_big->b_encryption;
            if (p_big->b_encryption)
                memcpy(create_sync.broadcast_code, broadcast_source.broadcast_code, sizeof(create_sync.broadcast_code));
            else
                memset(create_sync.broadcast_code, 0, sizeof(create_sync.broadcast_code));
        WICED_BT_TRACE("[BIG] b_base_updated=%d b_biginfo_updated=%d sub_group_cnt=%d bis_cnt=%d\n",
       p_big->b_base_updated, p_big->b_biginfo_updated,
       p_big->base.sub_group_cnt, p_big->base.sub_group[0].bis_cnt);
            wiced_result_t res = wiced_ble_isoc_peripheral_big_create_sync(&create_sync);
            (void)res;
            WICED_BT_TRACE("[BIG] create_sync called: result=%d num_bis=%d big_handle=%d sync_handle=%d\n",
       res, create_sync.num_bis, create_sync.big_handle, create_sync.sync_handle);
            WICED_BT_TRACE("[%s] result %x\n", __FUNCTION__, res);
    }

    return;
}

void broadcast_sink_ext_adv_cback(wiced_ble_ext_adv_event_t event, wiced_ble_ext_adv_event_data_t *p_ed)
{
    broadcast_sink_cb_t *p_big = NULL;
    wiced_ble_ext_scan_results_t ext_adv_report = {0};

    // WICED_BT_TRACE("[%s] event [%d]\n", __FUNCTION__, event);
    WICED_BT_TRACE("[EXT_ADV] event=%d\n", event);
    switch (event)
    {
    case WICED_BLE_PERIODIC_ADV_SYNC_TRANSFER_EVENT:

        WICED_BT_TRACE("[%s] WICED_BLE_PERIODIC_ADV_SYNC_TRANSFER_EVENT adv sid %d sync_handle %d BDA %B",
                       __FUNCTION__,
                       p_ed->sync_transfer.sync_data.adv_sid,
                       p_ed->sync_transfer.sync_data.sync_handle,
                       p_ed->sync_transfer.sync_data.adv_addr);

        broadcast_sink_bis_update_sync_handle(p_ed->sync_transfer.sync_data.adv_addr,
                                              p_ed->sync_transfer.sync_data.adv_sid,
                                              p_ed->sync_transfer.sync_data.sync_handle);

        broadcast_sink_bass_notify_pa_sync_state(&p_ed->sync_transfer.sync_data);
        break;

    case WICED_BLE_PERIODIC_ADV_SYNC_ESTABLISHED_EVENT:

        WICED_BT_TRACE("[%s] PERIODIC_ADV_SYNC_ESTABLISHED status 0x%x sync_handle %d BDA %B",
                       __FUNCTION__,
                       p_ed->sync_establish.status,
                       p_ed->sync_establish.sync_handle,
                       p_ed->sync_establish.adv_addr);
         WICED_BT_TRACE("[PA] SYNC_ESTABLISHED status=0x%x sync_handle=%d\n",
           p_ed->sync_establish.status, p_ed->sync_establish.sync_handle);
        p_big = broadcast_sink_bis_get_big_by_adv_sid(p_ed->sync_establish.adv_addr, p_ed->sync_establish.adv_sid);
        if (p_big)
        {
            p_big->sync_in_progress = WICED_FALSE;
        }
        // Stop any scan operations
        //wiced_bt_ble_observe(WICED_FALSE, 0, NULL);
        wiced_ble_ext_scan_enable(WICED_FALSE, NULL);

        /* Handle error status */
        if (p_ed->sync_establish.status)
        { printf(">> Error: Periodic advertising synchronization failed (0x%02x). Retrying...\n",
             p_ed->sync_establish.status);
            broadcast_sink_periodic_sync_in_progress = FALSE;
            ext_adv_report.adv_sid = p_ed->sync_establish.adv_sid;
            ext_adv_report.ble_addr_type = p_ed->sync_establish.adv_addr_type;
            memcpy(ext_adv_report.remote_bd_addr, p_ed->sync_establish.adv_addr, WICED_BT_ADDRESS_BYTE_SIZE);
            ext_adv_report.periodic_adv_interval = p_ed->sync_establish.periodic_adv_int;
            broadcast_sink_create_periodic_sync(&ext_adv_report);
            return;
        }
        else{ printf("Periodic advertising synchronized. Waiting for audio configuration...\n");}

       // le_audio_rpc_send_status_update(PA_SYNC_ESTABLISHED);

        /* map sync_handle => (addr, adv_sid) */
        broadcast_sink_bis_update_sync_handle(p_ed->sync_establish.adv_addr,
                                              p_ed->sync_establish.adv_sid,
                                              p_ed->sync_establish.sync_handle);

        broadcast_sink_bass_notify_pa_sync_state(&p_ed->sync_establish);

        /* Issue further sync commands if pending */
        broadcast_sink_create_periodic_sync(NULL);
        break;

    case WICED_BLE_PERIODIC_ADV_REPORT_EVENT:
    case WICED_BLE_BIGINFO_ADV_REPORT_EVENT:
        priv_periodic_adv_report_handler(event, p_ed);
        break;

    case WICED_BLE_PERIODIC_ADV_SYNC_LOST_EVENT:
        printf("Periodic advertising synchronization lost.\n");
       // le_audio_rpc_send_status_update(PA_SYNC_LOST);
        broadcast_sink_bass_notify_pa_sync_lost(p_ed->sync_handle);
    default:
        break;
    }
}

void broadcast_sink_bis_isoc_cb(wiced_ble_isoc_event_t event, wiced_ble_isoc_event_data_t *p_ed)
{
    broadcast_sink_cb_t *p_big = NULL;
    wiced_ble_isoc_terminated_evt_t *p_big_sync_lost = NULL;
    wiced_bt_ga_bap_csc_t *p_csc = NULL;
    wiced_ble_isoc_big_sync_established_evt_t *p_big_sync_established = &p_ed->big_sync_established;
    uint8_t lc3_index[2] = {0};

    WICED_BT_TRACE("[%s] event %d ", __FUNCTION__, event);

    switch (event)
    {
    case WICED_BLE_ISOC_BIG_SYNC_ESTABLISHED_EVT: {
        if (p_big_sync_established->status)
        {
            printf(">> Error: Broadcast audio synchronization failed (0x%02x).\n",
                   p_big_sync_established->status);
            return;
        }

        p_big = broadcast_sink_bis_get_big_by_handle(p_big_sync_established->big_handle);
        if (!p_big) return;

         printf("Broadcast audio synchronized. Broadcast ID: 0x%06lx\n",
             (unsigned long)p_big->base.broadcast_id);

        //le_audio_rpc_send_status_update(BIG_SYNC_ESTABLISHED);
        // TODO: support selecting  a particular BIS instead of hardcoding to 0
        p_csc = &p_big->base.sub_group[0].csc;

        // Stop any scan operations
        //wiced_bt_ble_observe(WICED_FALSE, 0, NULL);
        wiced_ble_ext_scan_enable(WICED_FALSE, NULL);

        //wiced_ble_padv_terminate_sync(p_big->sync_handle);

        memcpy(p_big->bis_conn_id_list,
               p_big_sync_established->bis_conn_hdl_list,
               p_big_sync_established->num_bis * sizeof(uint16_t));

        if (WICED_SUCCESS != iso_audio_setup_data_path(p_big->bis_conn_id_list[0],
                                  WICED_BLE_ISOC_DPD_OUTPUT_BIT,
                                  p_csc,
                                  p_big->base.sub_group[0].bis_cnt))
        {
            printf(">> Error: Broadcast audio data path setup failed.\n");
            return;
        }
        printf("Audio configuration: %lu Hz, %lu us, %u bytes/frame, %u BIS\n",
               (unsigned long)p_csc->sampling_frequency,
               (unsigned long)p_csc->frame_duration,
               p_csc->octets_per_codec_frame,
               p_big_sync_established->num_bis);
        audio_driver_set_volume((DEFAULT_VOL * 100) / 255);

            /* If stereo (2 BIS), open a raw HCI data path for BIS[1] (R channel)
     * so the controller delivers its packets to the host for inspection. */
    if (p_big_sync_established->num_bis >= 2)
    {
        wiced_ble_isoc_setup_data_path_info_t r_path = {0};
        r_path.isoc_conn_hdl    = p_big->bis_conn_id_list[1];
        r_path.data_path_dir    = WICED_BLE_ISOC_DPD_OUTPUT;
        r_path.data_path_id     = WICED_BLE_ISOC_DPID_HCI;
        r_path.controller_delay = 0;
        r_path.csc_length       = 0;
        r_path.p_csc            = NULL;
        r_path.p_app_ctx        = NULL;
        WICED_BT_TRACE("[BIS_R] Setting up raw RX data path for R-channel conn_hdl=0x%04x\n",
               p_big->bis_conn_id_list[1]);
        wiced_ble_isoc_setup_data_path(&r_path);
    }

        broadcast_sink_bass_notify_sync_established(p_big->bd_addr);
    }
    break;

    case WICED_BLE_ISOC_BIG_SYNC_LOST_EVT: {
        p_big_sync_lost = &p_ed->big_sync_lost;
    printf("Broadcast audio synchronization lost (handle %d, reason 0x%02x).\n",
       p_big_sync_lost->big_handle, p_big_sync_lost->reason);

    p_big = broadcast_sink_bis_get_big_by_handle(p_big_sync_lost->big_handle);
    if (!p_big) return;

    /* Save broadcast_id from the slot BEFORE it is zeroed */
    uint32_t saved_broadcast_id = p_big->base.broadcast_id;

    broadcast_sink_bass_notify_big_sync_lost(p_big->bd_addr);
    iso_audio_remove_data_path(p_big->bis_conn_id_list[0], WICED_BLE_ISOC_DPD_OUTPUT_BIT, lc3_index);
    broadcast_sink_bis_free_big(p_big);

    /* Restore broadcast_source so ext_adv_scan_to_sync_cback can filter by it */
    broadcast_source.broadcast_id = saved_broadcast_id;

    /* Restart scan to re-detect the source when it broadcasts again */
    broadcast_sink_periodic_sync_in_progress = FALSE;
    {
        wiced_ble_ext_scan_params_t scan_params = {0};
        scan_params.scanning_phys = WICED_BLE_EXT_ADV_PHY_1M_BIT;
        scan_params.sp_1m.scan_interval = 0x0060;
        scan_params.sp_1m.scan_window   = 0x0030;
        wiced_ble_ext_scan_set_params(&scan_params);

        wiced_ble_ext_scan_enable_params_t sce = {0};
        wiced_ble_ext_scan_register_cb(broadcast_sink_bis_ext_adv_scan_to_sync_cback);
        wiced_ble_ext_scan_enable(WICED_TRUE, &sce);
        printf("Scanning to reconnect to Broadcast ID: 0x%06lx\n",
               (unsigned long)broadcast_source.broadcast_id);
    }
       // le_audio_rpc_send_status_update(BIG_SYNC_LOST);
       // WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_big->base.state);
    }
    break;

    default:
        break;
    }
}

/******************************************************************************
 * Function Name: broadcast_sink_bis_init
 *******************************************************************************
 * Summary:
 *   Initiate bis
 *
 * Parameters:
 *   wiced_bt_cfg_isoc_t *p_isoc_cfg    : isoc config
 *
 * Return:
 *
 ******************************************************************************/
extern wiced_ble_isoc_cfg_t broadcast_sink_isoc_cfg;   // ← move here (file scope)

void broadcast_sink_bis_init(wiced_bt_cfg_isoc_t *p_isoc_cfg)
{
    WICED_BT_TRACE("[%s] \n", __FUNCTION__);
    wiced_ble_isoc_init(&broadcast_sink_isoc_cfg, broadcast_sink_bis_isoc_cb);  // 1st
    wiced_ble_ext_adv_register_cback(broadcast_sink_ext_adv_cback);             // 2nd
    iso_audio_init(p_isoc_cfg);
}
/******************************************************************************
 * Function Name: broadcast_sink_bis_alloc_big
 *******************************************************************************
 * Summary:
 *   Allocate big
 *
 * Parameters:
 *   uint32_t broadcast_id              : broadcast ID
 *   wiced_bt_device_address_t bd_addr  : bd address
 *   uint8_t adv_sid                    : Advertising set identifier(SID)
 *
 * Return:
 *  broadcast_sink_cb_t : information structure
 *
 ******************************************************************************/
broadcast_sink_cb_t *broadcast_sink_bis_alloc_big(uint32_t broadcast_id,
                                                  wiced_bt_device_address_t bd_addr,
                                                  uint8_t adv_sid)
{
    broadcast_sink_cb_t *p_big = NULL;

    /* If BIG already exists do not allocate a new one */
    p_big = broadcast_sink_bis_get_big_by_broadcast_id(broadcast_id);
    if (p_big) return p_big;

    /* find a free slot */
    for (size_t i = 0; i < MAX_BIG; i++)
    {
        if (TRUE != g_broadcast_sink_cb[i].in_use)
        {
            p_big = &g_broadcast_sink_cb[i];
            break;
        }
    }

    if (!p_big) return p_big;

    p_big->in_use = TRUE;
    p_big->base.broadcast_id = broadcast_id;
    p_big->adv_handle = adv_sid;
    /* Adv. set ID will be unique per BASE, so it should be ok to use the same as BIG handle */
    p_big->big_handle = adv_sid;
    memcpy(p_big->bd_addr, bd_addr, BD_ADDR_LEN);

    p_big->sync_handle = 0xFF;
    p_big->base.state = BAP_BROADCAST_STATE_IDLE;
    p_big->b_encryption = FALSE;

    WICED_BT_TRACE("[%s] Initializing [State:%d] [br_id:0x%x] [adv_sid:%d] [sync_handle:0xFF]\n",
                   __FUNCTION__,
                   p_big->base.state,
                   p_big->base.broadcast_id,
                   adv_sid);

    return p_big;
}

/******************************************************************************
 * Function Name: broadcast_sink_bis_free_big
 *******************************************************************************
 * Summary:
 *   Free big
 *
 * Parameters:
 *   broadcast_sink_cb_t *p_big : big to be freed
 *
 * Return:
 *
 ******************************************************************************/
void broadcast_sink_bis_free_big(broadcast_sink_cb_t *p_big)
{
    memset(p_big, 0, sizeof(broadcast_sink_cb_t));
}

void broadcast_sink_create_periodic_sync(wiced_ble_ext_scan_results_t *p_ext_adv_report)
{
    /* Queue to hold "sync to periodic adv" requests (since controller rejects more than one request at a time) */
    static wiced_bt_buffer_q_t broadcast_sink_pending_scan_q = {0};
    wiced_ble_ext_scan_results_t *p_cached_scan_result = NULL;
    uint16_t sync_timeout = 0;

    /* If previous sync is pending still add this req to queue else controller will reject the command */
    if (broadcast_sink_periodic_sync_in_progress && p_ext_adv_report)
    {
        wiced_ble_ext_scan_results_t *p_ext_adv_report_cpy =
            (wiced_ble_ext_scan_results_t *)wiced_bt_get_buffer(sizeof(wiced_ble_ext_scan_results_t));

        *p_ext_adv_report_cpy = *p_ext_adv_report;

        /* add the request to queue */
        wiced_bt_enqueue(&broadcast_sink_pending_scan_q, p_ext_adv_report_cpy);
        WICED_BT_TRACE("[%s] Queuing periodic adv sync request\n", __FUNCTION__);
        return;
    }

    if (!p_ext_adv_report)
    {
        /* get pending sync request from queue */
        p_cached_scan_result = (wiced_ble_ext_scan_results_t *)wiced_bt_dequeue(&broadcast_sink_pending_scan_q);
        if (!p_cached_scan_result)
        {
            broadcast_sink_periodic_sync_in_progress = FALSE;
            return;
        }

        p_ext_adv_report = p_cached_scan_result;
    }

    WICED_BT_TRACE("[%s] Trying to sync to periodic adv\n", __FUNCTION__);
    /* The Sync timeout should at least be 6 times the interval to accommodate for 6 opportunities to catch the peer. */
    sync_timeout = (p_ext_adv_report->periodic_adv_interval * 3) / 4;
    wiced_ble_padv_create_sync_params_t sync_params = {
        .options       = WICED_BLE_PADV_CREATE_SYNC_OPTION_IGNORE_PA_LIST,
        .adv_sid       = p_ext_adv_report->adv_sid,
        .adv_addr_type = p_ext_adv_report->ble_addr_type,
        .skip          = 0,
        .sync_timeout  = sync_timeout,
        .sync_cte_type = 0
    };
    memcpy(sync_params.adv_addr, p_ext_adv_report->remote_bd_addr, BD_ADDR_LEN);
    wiced_ble_padv_create_sync(&sync_params);

    broadcast_sink_periodic_sync_in_progress = TRUE;

    if (p_cached_scan_result) wiced_bt_free_buffer(p_cached_scan_result);
}

void broadcast_sink_bis_ext_adv_scan_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_data_len, uint8_t *p_adv_data)
{
    uint32_t br_id = 0;
 WICED_BT_TRACE("[SCAN] adv received, periodic_interval=%d\n", p_scan_result ? p_scan_result->periodic_adv_interval : -1);
    if (!p_scan_result || p_scan_result->periodic_adv_interval == 0) return;

    if (!le_audio_bap_broadcast_is_broadcast_announcement(p_scan_result->adv_len, p_adv_data, &br_id)) return;
    printf("[SCAN] Found broadcast device: addr=%02X:%02X:%02X:%02X:%02X:%02X  br_id=0x%lx\n",
    p_scan_result->remote_bd_addr[5], p_scan_result->remote_bd_addr[4],
    p_scan_result->remote_bd_addr[3], p_scan_result->remote_bd_addr[2],
    p_scan_result->remote_bd_addr[1], p_scan_result->remote_bd_addr[0],
    (unsigned long)br_id);
    le_audio_rcv_public_broadcast_t p_rcv_br;
    if (le_audio_pbp_is_public_broadcast(p_scan_result->adv_len, p_adv_data, &p_rcv_br))
    {
        WICED_BT_TRACE("%s", p_rcv_br.broadcast_name);
        WICED_BT_TRACE("%d", p_rcv_br.audio_config);
        WICED_BT_TRACE("%d", p_rcv_br.encryption);
        WICED_BT_TRACE("%d", p_rcv_br.metadata_length);
        WICED_BT_TRACE("%d", p_rcv_br.source_appearance_value);
    }

    WICED_BT_TRACE("[%s] broadcast id found %x\n", __FUNCTION__, br_id);

   WICED_BT_TRACE("[%s] New stream found: br_id 0x%x name %s\n", __FUNCTION__, br_id, p_rcv_br.broadcast_name);
}


void broadcast_sink_bis_ext_adv_scan_to_sync_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_data_len, uint8_t *p_adv_data)
{
    uint32_t br_id = 0;
    broadcast_sink_cb_t *p_big = NULL;

    if (!p_scan_result || p_scan_result->periodic_adv_interval == 0) return;

    if (!le_audio_bap_broadcast_is_broadcast_announcement(p_scan_result->adv_len, p_adv_data, &br_id)) return;
    le_audio_rcv_public_broadcast_t p_rcv_br;

    if (!le_audio_pbp_is_public_broadcast(p_scan_result->adv_len, p_adv_data, &p_rcv_br))
    {

        WICED_BT_TRACE("%s", p_rcv_br.broadcast_name);
        WICED_BT_TRACE("%d", p_rcv_br.audio_config);
        WICED_BT_TRACE("%d", p_rcv_br.encryption);
        WICED_BT_TRACE("%d", p_rcv_br.metadata_length);
        WICED_BT_TRACE("%d", p_rcv_br.source_appearance_value);
    }
    if (br_id != broadcast_source.broadcast_id)
    {
        WICED_BT_TRACE("[%s] broadcast id found %x looking for %x \n",
                       __FUNCTION__,
                       br_id,
                       broadcast_source.broadcast_id);
        return;
    }

    // check for duplicate reports
    //TODO: Check for addr and br_id ?
    p_big = broadcast_sink_bis_get_big_by_broadcast_id(br_id);
    if (p_big == NULL)
    {
        // Alloc a slot to store BASE, do not sync to PA if unsuccessful
        p_big = broadcast_sink_bis_alloc_big(br_id, p_scan_result->remote_bd_addr, p_scan_result->adv_sid);
    }
    if (!p_big)
        return;

    WICED_BT_TRACE("[%s] sync_handle %d\n", __FUNCTION__, p_big->sync_handle);

    if ( (0xFF == p_big->sync_handle)  && (p_big->sync_in_progress == WICED_FALSE))
    {
        p_big->sync_in_progress = WICED_TRUE;
        broadcast_sink_create_periodic_sync(p_scan_result);
    }
}

/******************************************************************************
 * Function Name: broadcast_sink_bis_menu_ext_adv_scan_cback
 *******************************************************************************
 * Summary:
 *   Callback for manually initiated extended advertising scans.
 *
 * Parameters:
 *   wiced_ble_ext_scan_results_t *p_scan_result : pointer to scan result
 *   uint16_t adv_data_len                     : advertising data length (unused)
 *   uint8_t *p_adv_data                        : pointer to adv data
 *
 * Return:
 *
 ******************************************************************************/
static void broadcast_sink_bis_menu_ext_adv_scan_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_data_len, uint8_t *p_adv_data)
{
    uint32_t br_id = 0;

    if (!p_scan_result || p_scan_result->periodic_adv_interval == 0)
    {
        /* Reject missing reports or a zero periodic interval. */
        return;
    }

    WICED_BT_TRACE("[SCAN] periodic device: addr=%02X:%02X:%02X:%02X:%02X:%02X  interval=%d\n",
        p_scan_result->remote_bd_addr[5], p_scan_result->remote_bd_addr[4],
        p_scan_result->remote_bd_addr[3], p_scan_result->remote_bd_addr[2],
        p_scan_result->remote_bd_addr[1], p_scan_result->remote_bd_addr[0],
        p_scan_result->periodic_adv_interval);

    if (!le_audio_bap_broadcast_is_broadcast_announcement(p_scan_result->adv_len, p_adv_data, &br_id))
    {
        /* Reject reports that do not contain a broadcast announcement. */
        return;
    }

    WICED_BT_TRACE("[SCAN] broadcast source: addr=%02X:%02X:%02X:%02X:%02X:%02X  br_id=0x%06lx\n",
        p_scan_result->remote_bd_addr[5], p_scan_result->remote_bd_addr[4],
        p_scan_result->remote_bd_addr[3], p_scan_result->remote_bd_addr[2],
        p_scan_result->remote_bd_addr[1], p_scan_result->remote_bd_addr[0],
        (unsigned long)br_id);
    le_audio_rcv_public_broadcast_t p_rcv_br;
    if (le_audio_pbp_is_public_broadcast(p_scan_result->adv_len, p_adv_data, &p_rcv_br))
    { WICED_BT_TRACE("[SCAN] Name: %s  config=%d  encrypted=%d\n",
            p_rcv_br.broadcast_name, p_rcv_br.audio_config, p_rcv_br.encryption);
        WICED_BT_TRACE("       broadcast ID[%x]         %4d           %4d             %4d              %4d                     %20s", br_id, p_rcv_br.audio_config, p_rcv_br.encryption, p_rcv_br.metadata_length, p_rcv_br.source_appearance_value, p_rcv_br.broadcast_name);
    }
// Alloc BIG slot and trigger PA sync directly — no scan restart needed
    broadcast_sink_cb_t *p_big = broadcast_sink_bis_get_big_by_broadcast_id(br_id);
    if (!p_big)
        p_big = broadcast_sink_bis_alloc_big(br_id, p_scan_result->remote_bd_addr, p_scan_result->adv_sid);
    if (!p_big) return;

    if ((0xFF == p_big->sync_handle) && (p_big->sync_in_progress == WICED_FALSE))
    {
        printf("Broadcast source found. Broadcast ID: 0x%06lx\n", (unsigned long)br_id);
        printf("Synchronizing to broadcast source...\n");
        p_big->sync_in_progress = WICED_TRUE;
        broadcast_sink_create_periodic_sync(p_scan_result);
    }}

/******************************************************************************
 * Function Name: broadcast_sink_bis_menu_discover_sources
 *******************************************************************************
 * Summary:
 *   Manually discover sources
 *
 * Parameters:
 *   bool start : start to discover sources
 *
 * Return:
 *
 ******************************************************************************/
void broadcast_sink_bis_menu_discover_sources(bool start)
{
    broadcast_sink_cb_t *p_big = NULL;
    uint8_t lc3_index[2] = {0};

    if (start)
    {
        for (size_t i = 0; i < MAX_BIG; i++)
        {
            if (g_broadcast_sink_cb[i].in_use == TRUE)
            {
                p_big = &g_broadcast_sink_cb[i];

                wiced_ble_isoc_peripheral_big_terminate_sync(p_big->big_handle);
                wiced_ble_padv_terminate_sync(p_big->sync_handle);
                iso_audio_remove_data_path(p_big->bis_conn_id_list[0], WICED_BLE_ISOC_DPD_OUTPUT_BIT, lc3_index);
                broadcast_sink_bis_free_big(p_big);
            }
        }
        broadcast_sink_clear_data();
    }

   wiced_ble_ext_scan_params_t scan_params = {0};
    scan_params.scanning_phys = WICED_BLE_EXT_ADV_PHY_1M_BIT;
    scan_params.sp_1m.scan_interval = 0x0060;
    scan_params.sp_1m.scan_window = 0x0030;
    wiced_ble_ext_scan_set_params(&scan_params);

    wiced_ble_ext_scan_enable_params_t sce = {0};
    wiced_ble_ext_scan_register_cb(start ? broadcast_sink_bis_menu_ext_adv_scan_cback : NULL);
    wiced_ble_ext_scan_enable(start, start ? &sce : NULL);

}

/******************************************************************************
 * Function Name: broadcast_sink_bis_discover_sources
 *******************************************************************************
 * Summary:
 *   Discover sources
 *
 * Parameters:
 *   wiced_bt_ble_scan_type_t scan_type : scan type
 *
 * Return:
 *
 ******************************************************************************/
void broadcast_sink_bis_discover_sources(wiced_bt_ble_scan_type_t scan_type)
{
    wiced_ble_ext_scan_params_t scan_params = {0};
    scan_params.scanning_phys = WICED_BLE_EXT_ADV_PHY_1M_BIT;
    scan_params.sp_1m.scan_interval = 0x0060;
    scan_params.sp_1m.scan_window = 0x0030;
    wiced_ble_ext_scan_set_params(&scan_params);

    wiced_ble_ext_scan_enable_params_t sce = {0};
    wiced_ble_ext_scan_register_cb(scan_type ? broadcast_sink_bis_ext_adv_scan_cback : NULL);
    wiced_ble_ext_scan_enable(scan_type ? WICED_TRUE : WICED_FALSE, scan_type ? &sce : NULL);

}

/******************************************************************************
 * Function Name: broadcast_sink_bis_sync_to_source
 *******************************************************************************
 * Summary:
 *   Sync to source
 *
 * Parameters:
 *   wiced_bt_ble_scan_type_t scan_type : scan type
 *   broadcast_source_t source          : source
 *
 * Return:
 *
 ******************************************************************************/
void broadcast_sink_bis_sync_to_source(wiced_bt_ble_scan_type_t scan_type, broadcast_source_t source)
{
    memcpy(&broadcast_source, &source, sizeof(broadcast_source_t));
        // Configure extended scanning for the selected broadcast source.

     wiced_ble_ext_scan_params_t scan_params = {0};
    scan_params.scanning_phys = WICED_BLE_EXT_ADV_PHY_1M_BIT;
    scan_params.sp_1m.scan_interval = 0x0060;
    scan_params.sp_1m.scan_window = 0x0030;
    wiced_ble_ext_scan_set_params(&scan_params);

    wiced_ble_ext_scan_enable_params_t sce = {0};
    wiced_ble_ext_scan_register_cb(scan_type ? broadcast_sink_bis_ext_adv_scan_to_sync_cback : NULL);
    wiced_ble_ext_scan_enable(scan_type ? WICED_TRUE : WICED_FALSE, scan_type ? &sce : NULL);
}

/******************************************************************************
 * Function Name: broadcast_sink_clear_data
 *******************************************************************************
 * Summary:
 *   Clear bis data
 *
 * Parameters:
 *
 * Return:
 *
 ******************************************************************************/
void broadcast_sink_clear_data()
{
    WICED_BT_TRACE("[%s] \n", __FUNCTION__);
    for (size_t i = 0; i < MAX_BIG; i++)
    {
        if (g_broadcast_sink_cb[i].in_use == TRUE)
        {
            WICED_BT_TRACE("[%s]terminating sync to  %B big handle %d\n",
                           __FUNCTION__,
                           g_broadcast_sink_cb[i].bd_addr,
                           g_broadcast_sink_cb[i].big_handle);
            wiced_result_t ret = wiced_ble_padv_terminate_sync(g_broadcast_sink_cb[i].sync_handle);
            WICED_BT_TRACE("[%s] terminate res %x \n", __FUNCTION__, ret);
            (void)ret;
        }
        memset(g_broadcast_sink_cb, 0, sizeof(g_broadcast_sink_cb));
    }

    broadcast_sink_periodic_sync_in_progress = FALSE;
}

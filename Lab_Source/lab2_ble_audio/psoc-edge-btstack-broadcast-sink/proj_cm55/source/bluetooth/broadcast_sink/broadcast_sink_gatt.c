/******************************************************************************
* File Name        : broadcast_sink_gatt.c
*
* Description      : Broadcast sink GATT database and event handling.
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
#include "broadcast_sink_gatt.h"
#include "broadcast_sink_bass.h"
#include "cy_pdl.h"
#include "cybsp.h"
/* App Library includes */
#include "gatt_interface.h"
#include "wiced_bt_ga_pacs.h"

/* BT Stack includes */

#include "wiced_bt_gatt.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_uuid.h"

#include "logging.h"


extern wiced_bt_cfg_ble_t broadcast_sink_ble_cfg;

static profiles_t g_broadcast_sink_local_profiles;
static broadcast_sink_gatt_cb_t g_broadcast_sink_gatt_cb;

enum
{
    HDLS_GENERIC_ATTRIBUTE = 1, // 0x0001 , 1

    HDLS_PACS = 208,                          // 0x00D0 , 208
    HDLC_PACS_SINK_PAC,                       // 0x00D1 , 209
    HDLC_PACS_SINK_PAC_VALUE,                 // 0x00D2 , 210
    HDLD_PACS_SINK_PAC_CCCD,                  // 0x00D3 , 211
    HDLC_PACS_SINK_AUDIO_LOCATIONS,           // 0x00D4 , 212
    HDLC_PACS_SINK_AUDIO_LOCATIONS_VALUE,     // 0x00D5 , 213
    HDLC_PACS_SINK_AUDIO_LOCATIONS_CCCD,      // 0x00D6 , 214
    HDLC_PACS_SOURCE_PAC,                     // 0x00D7 , 215
    HDLC_PACS_SOURCE_PAC_VALUE,               // 0x00D8 , 216
    HDLD_PACS_SOURCE_PAC_CCCD,                // 0x00D9 , 217
    HDLC_PACS_SOURCE_AUDIO_LOCATIONS,         // 0x00DA , 218
    HDLC_PACS_SOURCE_AUDIO_LOCATIONS_VALUE,   // 0x00DB , 219
    HDLC_PACS_SOURCE_AUDIO_LOCATIONS_CCCD,    // 0x00DC , 220
    HDLC_PACS_AVAILABLE_AUDIO_CONTEXTS,       // 0x00DD , 221
    HDLC_PACS_AVAILABLE_AUDIO_CONTEXTS_VALUE, // 0x00DE , 222
    HDLD_PACS_AVAILABLE_AUDIO_CONTEXTS_CCCD,  // 0x00DF , 223
    HDLC_PACS_SUPPORTED_AUDIO_CONTEXTS,       // 0x00E0 , 224
    HDLC_PACS_SUPPORTED_AUDIO_CONTEXTS_VALUE, // 0x00E1 , 225
    HDLC_PACS_SUPPORTED_AUDIO_CONTEXTS_CCCD,  // 0x00E2 , 226

    HDLS_BASS = 500,                                          // 0x01F4 , 500
    HDLC_BASS_BROADCAST_AUDIO_SCAN_CONTROL_POINT,             // 0x01F5 , 501
    HDLC_BASS_BROADCAST_AUDIO_SCAN_CONTROL_POINT_VALUE,       // 0x01F6 , 502
    HDLC_BASS_BROADCAST_RECEIVE_STATE,                        // 0x01F7 , 503
    HDLC_BASS_BROADCAST_RECEIVE_STATE_VALUE,                  // 0x01F8 , 504
    HDLD_BASS_BROADCAST_RECEIVE_STATE_CLIENT_CONFIGURATION,   // 0x01F9 , 505
    HDLC_BASS_BROADCAST_RECEIVE_STATE_2,                      // 0x01FA , 506
    HDLC_BASS_BROADCAST_RECEIVE_STATE_VALUE_2,                // 0x01FB , 507
    HDLD_BASS_BROADCAST_RECEIVE_STATE_CLIENT_CONFIGURATION_2, // 0x01FC , 508
};

const uint8_t broadcast_sink_gatt_database[] = {

    /* Primary Service 'Generic Attribute' */
    PRIMARY_SERVICE_UUID16(HDLS_GENERIC_ATTRIBUTE, UUID_SERVICE_GATT),

    /* Primary Service 'Published Audio Capability Service' */
    PRIMARY_SERVICE_UUID16(HDLS_PACS, WICED_BT_UUID_PUBLISHED_AUDIO_CAPABILITY),

    CHARACTERISTIC_UUID16(HDLC_PACS_SINK_PAC,
                          HDLC_PACS_SINK_PAC_VALUE,
                          WICED_BT_UUID_PACS_SINK_PAC,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE),

    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_PACS_SINK_PAC_CCCD,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD),

    CHARACTERISTIC_UUID16_WRITABLE(HDLC_PACS_SINK_AUDIO_LOCATIONS,
                                   HDLC_PACS_SINK_AUDIO_LOCATIONS_VALUE,
                                   WICED_BT_UUID_PACS_SINK_AUDIO_LOCATIONS,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ),

    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLC_PACS_SINK_AUDIO_LOCATIONS_CCCD,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD),

    CHARACTERISTIC_UUID16(HDLC_PACS_SOURCE_PAC,
                          HDLC_PACS_SOURCE_PAC_VALUE,
                          WICED_BT_UUID_PACS_SOURCE_PAC,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE),

    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_PACS_SOURCE_PAC_CCCD,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD),

    CHARACTERISTIC_UUID16_WRITABLE(HDLC_PACS_SOURCE_AUDIO_LOCATIONS,
                                   HDLC_PACS_SOURCE_AUDIO_LOCATIONS_VALUE,
                                   WICED_BT_UUID_PACS_SOURCE_AUDIO_LOCATIONS,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ),

    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLC_PACS_SOURCE_AUDIO_LOCATIONS_CCCD,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD),

    CHARACTERISTIC_UUID16(HDLC_PACS_AVAILABLE_AUDIO_CONTEXTS,
                          HDLC_PACS_AVAILABLE_AUDIO_CONTEXTS_VALUE,
                          WICED_BT_UUID_PACS_AUDIO_CONTEXT_AVAILABILITY,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE),

    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_PACS_AVAILABLE_AUDIO_CONTEXTS_CCCD,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD),

    CHARACTERISTIC_UUID16(HDLC_PACS_SUPPORTED_AUDIO_CONTEXTS,
                          HDLC_PACS_SUPPORTED_AUDIO_CONTEXTS_VALUE,
                          WICED_BT_UUID_PACS_SUPPORTED_AUDIO_CONTEXT,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE),

    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLC_PACS_SUPPORTED_AUDIO_CONTEXTS_CCCD,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD),

    /* Primary Service 'BASS' */
    PRIMARY_SERVICE_UUID16(HDLS_BASS, WICED_BT_UUID_BROADCAST_AUDIO_SCAN),

    /* Characteristic 'Broadcast Audio Scan Control Point' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_BASS_BROADCAST_AUDIO_SCAN_CONTROL_POINT,
                                   HDLC_BASS_BROADCAST_AUDIO_SCAN_CONTROL_POINT_VALUE,
                                   WICED_BT_UUID_BASS_CONTROL_POINT,
                                   GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                   GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE),

    /* Characteristic 'Broadcast Receive State' */
    CHARACTERISTIC_UUID16(HDLC_BASS_BROADCAST_RECEIVE_STATE,
                          HDLC_BASS_BROADCAST_RECEIVE_STATE_VALUE,
                          WICED_BT_UUID_BASS_BROADCAST_RECEIVE_STATE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_BASS_BROADCAST_RECEIVE_STATE_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD |
                                        GATTDB_PERM_AUTH_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Broadcast Receive State' */
    CHARACTERISTIC_UUID16(HDLC_BASS_BROADCAST_RECEIVE_STATE_2,
                          HDLC_BASS_BROADCAST_RECEIVE_STATE_VALUE_2,
                          WICED_BT_UUID_BASS_BROADCAST_RECEIVE_STATE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_BASS_BROADCAST_RECEIVE_STATE_CLIENT_CONFIGURATION_2,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD |
                                        GATTDB_PERM_AUTH_WRITABLE | GATTDB_PERM_AUTH_READABLE),

};

void broadcast_sink_gatt_handle_connection(wiced_bt_gatt_connection_status_t *p_conn_sts)
{
        printf("Broadcast Assistant connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
            p_conn_sts->bd_addr[0], p_conn_sts->bd_addr[1], p_conn_sts->bd_addr[2],
            p_conn_sts->bd_addr[3], p_conn_sts->bd_addr[4], p_conn_sts->bd_addr[5]);
    WICED_BT_TRACE("[%s]: Connected to %B\n", __FUNCTION__, p_conn_sts->bd_addr);

   wiced_ble_padv_sync_transfer_param_t st_params = {
        .skip         = 0,
        .sync_timeout = 5000,
        .mode         = WICED_BLE_PERIODIC_ENABLE_PA_SYNC_TRANSFER_ENABLE_PA_REPORT_EVT,
        .sync_cte_type = 0
    };

    wiced_ble_padv_set_sync_transfer_params(p_conn_sts->bd_addr, &st_params);



   Cy_GPIO_Write(CYBSP_LED_GREEN_PORT, CYBSP_LED_GREEN_PIN, CYBSP_LED_STATE_OFF);

}

void broadcast_sink_gatt_handle_disconnection(wiced_bt_gatt_connection_status_t *p_conn_sts)
{

        printf("Broadcast Assistant disconnected: %02X:%02X:%02X:%02X:%02X:%02X\n",
            p_conn_sts->bd_addr[0], p_conn_sts->bd_addr[1], p_conn_sts->bd_addr[2],
            p_conn_sts->bd_addr[3], p_conn_sts->bd_addr[4], p_conn_sts->bd_addr[5]);
    WICED_BT_TRACE("[%s]: Disconnected from %B\n", __FUNCTION__, p_conn_sts->bd_addr);

}

wiced_bt_gatt_status_t broadcast_sink_gatt_cback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    WICED_BT_TRACE("[%s] event [%d]\n", __FUNCTION__, event);

    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT: {
            if(p_event_data->connection_status.connected){
                broadcast_sink_gatt_handle_connection(&p_event_data->connection_status);
            }else{
                broadcast_sink_gatt_handle_disconnection(&p_event_data->connection_status);
            }
        } break;

        default:
            break;
    }

    /* invoke the gatt interface common handler*/
    if(status == WICED_BT_GATT_SUCCESS){
        status = gatt_interface_invoke_gatt_handler(event, p_event_data);
    }

    return status;
}

wiced_bool_t broadcast_sink_check_to_save(void *p_app_ctx, wiced_bt_uuid_t *p_uuid)
{
    wiced_bool_t ret_val = FALSE;

    /* This app supports only 16 bit UUID */
    if (LEN_UUID_16 != p_uuid->len) return ret_val;

    switch (p_uuid->uu.uuid16)
    {
        case WICED_BT_UUID_PUBLISHED_AUDIO_CAPABILITY:
            ret_val = TRUE;
            break;
        case WICED_BT_UUID_BROADCAST_AUDIO_SCAN:
            ret_val = TRUE;
            break;
        default:
            break;
    }

    return ret_val;
}

void broadcast_sink_store_service_ref(void *p_app_ctx, wiced_bt_uuid_t *p_uuid, gatt_intf_service_object_t *p_service)
{
    /* This app supports only 16 bit UUID */
    if (LEN_UUID_16 != p_uuid->len) return;

    switch (p_uuid->uu.uuid16)
    {
        case WICED_BT_UUID_PUBLISHED_AUDIO_CAPABILITY:
            /* TODO: Add PACS Callback */
            WICED_BT_TRACE("[%s] PACS \n", __FUNCTION__);
            ((profiles_t *)p_app_ctx)->p_pacs = p_service;
            gatt_interface_set_callback_to_profile(p_service, NULL, p_app_ctx);
            break;
        case WICED_BT_UUID_BROADCAST_AUDIO_SCAN:
            WICED_BT_TRACE("[%s] BASS \n", __FUNCTION__);
            ((profiles_t *)p_app_ctx)->p_bass = p_service;
            gatt_interface_set_callback_to_profile(p_service, (gatt_intf_service_cb_t)broadcast_sink_bass_callback, p_app_ctx);
        default:
            break;
    }
}

wiced_bt_gatt_status_t broadcast_sink_gatt_init(int max_connections, int max_mtu, ga_cfg_t *p_ga_cfg)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = {0};

    gatt_status = wiced_bt_gatt_db_init(broadcast_sink_gatt_database, sizeof(broadcast_sink_gatt_database), NULL);
    if (WICED_BT_SUCCESS != gatt_status) return gatt_status;

    gatt_status = wiced_bt_gatt_register(broadcast_sink_gatt_cback);
    if (WICED_BT_SUCCESS != gatt_status) return gatt_status;

    /* Initialize GATT Interface App library */
    gatt_status = gatt_interface_init(max_connections, max_mtu, GATT_AUTH_REQ_NONE);
    if (WICED_BT_SUCCESS != gatt_status) return gatt_status;

    /* Initialize the supported profiles */
    wiced_bt_ga_pacs_init(p_ga_cfg);
    wiced_bt_ga_bass_init(p_ga_cfg);

    gatt_interface_setup_services_from_local_db(broadcast_sink_check_to_save,
                                                broadcast_sink_store_service_ref,
                                                &g_broadcast_sink_local_profiles);
    gatt_interface_print_linked_handles(bda);

    return gatt_status;
}

broadcast_sink_gatt_cb_t *broadcast_sink_gatt_alloc_cb(uint8_t *p_bd_addr,
                                                       wiced_bt_ble_address_type_t addr_type,
                                                       uint16_t conn_id,
                                                       uint16_t link_role)
{
    broadcast_sink_gatt_cb_t *p_gatt_cb = &g_broadcast_sink_gatt_cb;

    p_gatt_cb->in_use = TRUE;
    p_gatt_cb->conn_id = conn_id;
    p_gatt_cb->addr_type = addr_type;
    memcpy(p_gatt_cb->bda, p_bd_addr, BD_ADDR_LEN);
    p_gatt_cb->b_is_central = (HCI_ROLE_CENTRAL == link_role) ? TRUE : FALSE;
    return p_gatt_cb;
}

wiced_bt_gatt_status_t broadcast_sink_gatt_free_cb(uint8_t *p_bd_addr)
{
    broadcast_sink_gatt_cb_t *p_gatt_cb = &g_broadcast_sink_gatt_cb;

    p_gatt_cb->in_use = FALSE;

    return WICED_SUCCESS;
}

broadcast_sink_gatt_cb_t *broadcast_sink_gatt_get_cb(uint8_t *p_bd_addr)
{
    broadcast_sink_gatt_cb_t *p_gatt_cb = &g_broadcast_sink_gatt_cb;

    return p_gatt_cb;
}

broadcast_sink_gatt_cb_t *broadcast_sink_gatt_get_cb_by_conn_id(uint16_t conn_id)
{
    broadcast_sink_gatt_cb_t *p_gatt_cb = &g_broadcast_sink_gatt_cb;

    return p_gatt_cb;
}

gatt_intf_service_object_t *broadcast_sink_gatt_get_bass_service_instance()
{
    return g_broadcast_sink_local_profiles.p_bass;
}

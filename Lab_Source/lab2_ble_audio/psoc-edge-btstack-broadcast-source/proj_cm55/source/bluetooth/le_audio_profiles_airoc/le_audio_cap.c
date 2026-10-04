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
#include "le_audio_cap.h"
#include "wiced_bt_ga_csip.h"
#include "wiced_memory.h"
#include "wiced_timer.h"
#include "gatt_interface.h"

//#define SKIP_CIS

typedef struct
{
    le_audio_cap_app_data_t *app_data;
    gatt_intf_attribute_t volume_characteristic;
    wiced_bt_ga_vcs_data_t vcs_data;
    wiced_bt_ga_vocs_data_t vocs_data;
    wiced_bt_ga_mics_data_t mics_data;
    wiced_bt_ga_aics_data_t aics_data;
} wiced_ga_cap_local_volume_data_t;

typedef struct
{
    le_audio_cap_app_data_t *app_data;
    le_audio_cap_start_unicast_param_t *params;
    int curr_index;
    wiced_bt_ga_ascs_state_t opcode;
} wiced_ga_cap_local_unicast_cb_data_t;

typedef struct
{
    le_audio_cap_cback_t *p_app_cback;
    wiced_bt_ga_csip_set_entry *p_csis_devices; // store temp pointer to save allocated memory for locking use case
    uint8_t enable_locking;
} ga_cap_csip_data_t;

ga_cap_csip_data_t cap_csip_data;


static wiced_bt_gatt_status_t cap_ascs_unlock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                           void *data,
                                                           wiced_bt_gatt_status_t status);
wiced_bool_t cap_start_csis_unlocking_procedure(le_audio_cap_app_data_t *app_data,
                                                wiced_bt_lock_procedure_cmpl_cb callback);
wiced_bool_t cap_start_csis_locking_procedure(le_audio_cap_app_data_t *app_data,
                                              wiced_bt_lock_procedure_cmpl_cb callback,
                                              void *data);
wiced_bt_gatt_status_t cap_volume_lock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                         void *data,
                                                         wiced_bt_gatt_status_t status);
wiced_bt_gatt_status_t cap_unlock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                           void *data,
                                                           wiced_bt_gatt_status_t status);
static wiced_result_t ga_cap_send_mics_command_helper(wiced_ga_cap_local_volume_data_t *volume_data);
static wiced_result_t ga_cap_send_vcs_command_helper(wiced_ga_cap_local_volume_data_t *volume_data);
static wiced_result_t ga_cap_send_mics_command(wiced_ga_cap_local_volume_data_t *volume_data);

void wiced_bt_ga_cap_disable_csis_locking(void)
{
    cap_csip_data.enable_locking = WICED_FALSE;
}

uint8_t wiced_bt_ga_cap_utils_get_ascs_state_count(le_audio_cap_app_data_t *p_cap_app_data, uint8_t state)
{
    int index;
    int count = 0;
    for (index = 0; index < p_cap_app_data->num_devices; index++)
    {
        if (p_cap_app_data->device_info_list[index].ascs_data[0]&&
            p_cap_app_data->device_info_list[index].ascs_data[0]->ase_state == state)
        {
            count++;
        }
    }
    return count;
}

wiced_bool_t wiced_bt_ga_cap_utils_is_ascs_state(le_audio_cap_app_data_t *p_cap_app_data, uint8_t state)
{
    int count = wiced_bt_ga_cap_utils_get_ascs_state_count(p_cap_app_data, state);
    if (count == p_cap_app_data->num_devices) return WICED_TRUE;
    return WICED_FALSE;
}

uint8_t wiced_bt_ga_cap_utils_get_cis_count(le_audio_cap_app_data_t *p_cap_app_data)
{
    uint8_t count = 0;
    int index;
    for (index = 0; index < p_cap_app_data->num_devices; index++)
    {
        count += p_cap_app_data->device_info_list[index].num_ase;
    }
    return count;
    //    return p_cap_app_data->num_devices;
}


static void ga_cap_invoke_app_cb(uint16_t conn_id, le_audio_cap_event_t event, le_audio_cap_event_data_t *p_event_data)
{
    if (cap_csip_data.p_app_cback) (*cap_csip_data.p_app_cback)(conn_id, event, p_event_data);
}

wiced_bool_t ga_cap_verify_context_type(uint8_t dir,
                                        uint16_t req_context,
                                        wiced_bt_ga_pacs_audio_contexts_t *supported,
                                        wiced_bt_ga_pacs_audio_contexts_t *available)
{
    if (!available || !supported)
    {
        CAP_TRACE("[%s] available or supported is 0 ", __FUNCTION__);
        return WICED_FALSE;
    }

    if (dir & WICED_BT_CAP_DIRECTION_SOURCE) // source
    {
        CAP_TRACE("[%s] supp_sink_contexts %d avlbl_sink_contexts %d req_context %d\n",
                  __FUNCTION__,
                  supported->sink_contexts,
                  available->sink_contexts,
                  req_context);

        // Server is audio sink
        if (!(supported->sink_contexts & req_context))
        {
            return WICED_FALSE;
        }
        if (!(available->sink_contexts & req_context))
        {
            // Not Available Error
            return WICED_FALSE;
        }
    }
    if (dir & WICED_BT_CAP_DIRECTION_SINK)
    {
        CAP_TRACE("[%s] WICED_BT_CAP_DIRECTION_SINK ", __FUNCTION__);
        // Server is audio source
        if (!(supported->source_contexts & req_context))
        {
            // Not Supported Error
            return WICED_FALSE;
        }
        if (!(available->source_contexts & req_context))
        {
            // Not Available Error
            return WICED_FALSE;
        }
    }
    return WICED_TRUE;
}

wiced_bool_t ga_cap_verify_audio_location(uint8_t dir,
                                          wiced_bt_ga_pacs_audio_location_t req,
                                          wiced_bt_ga_pacs_data_t *pacs_data)
{
    if (req != BAP_AUDIO_LOCATION_NOT_ALLOWED)
    {
        if (dir & WICED_BT_CAP_DIRECTION_SOURCE)
        {
            if (!(req & pacs_data->sink_audio_location)) return WICED_FALSE;
        }
        if (dir & WICED_BT_CAP_DIRECTION_SINK)
        {
            if (!(req & pacs_data->source_audio_location)) return WICED_FALSE;
        }
    }
    return WICED_TRUE;
}

uint8_t ga_cap_get_num_bit_set(uint32_t val)
{
    uint8_t count = 1;

    if (val == 0) return val;

    while (val > 0)
    {
        val &= (val - 1);
        count++;
    }
    return count;
}

void ga_cap_fill_lc3_codec_param(uint8_t *data, uint8_t len, le_audio_cap_codec_param_t *output)
{
    uint8_t type = 0;
    while (len > 2)
    {
        data++;
        len--;
        type = *data;
        data++;
        len--;
        switch (type)
        {
        case BAP_CODEC_CAPABILITIES_SUPPORTED_SAMPLING_FREQUENCIES_TYPE:
            output->sf = *data;
            data++;
            output->sf |= ((*data) << 8);
            data++;
            len -= 2;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_FRAME_DURATIONS_TYPE:
            output->frame_duration = *data;
            data++;
            len--;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_AUDIO_CHANNEL_COUNTS_TYPE:
            output->audio_ch_count = *data;
            data++;
            len--;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE:
            output->min_data_per_frame = *data;
            data++;
            output->min_data_per_frame |= ((*data) << 8);
            data++;
            output->max_data_per_frame = *data;
            data++;
            output->max_data_per_frame |= ((*data) << 8);
            data++;
            len -= 4;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU_TYPE:
            output->frame_per_sdu = *data;
            data++;
            len--;
            break;
        }
    }
    CAP_TRACE("[%s] sf %d frame_duration %d ch_count %d octet_min %d octet_max %d frame_per_sdu %d",
              __FUNCTION__,
              output->sf,
              output->frame_duration,
              output->audio_ch_count,
              output->min_data_per_frame,
              output->max_data_per_frame,
              output->frame_per_sdu);
}

wiced_bool_t ga_cap_compare_codec_param(le_audio_cap_codec_param_t *codec_param,
                                        wiced_bt_ga_ascs_config_codec_args_t *codec_arg)
{
    uint8_t num_ch = 1;
    uint32_t block_per_sdu = 0;

    if (codec_arg->csc.octets_per_codec_frame)
    {
        // verify octed per frame
        if ((codec_param->min_data_per_frame > codec_arg->csc.octets_per_codec_frame) ||
            (codec_param->max_data_per_frame < codec_arg->csc.octets_per_codec_frame))
        {
            CAP_TRACE("BAP_CODEC_CONFIG_OCTET_PER_CODEC_FRAME_TYPE Error. OPF %d requested min %d max %d ",
                      codec_arg->csc.octets_per_codec_frame,
                      codec_param->min_data_per_frame,
                      codec_param->max_data_per_frame);
            return WICED_FALSE;
        }
    }

    if (codec_arg->csc.sampling_frequency)
    {
        uint8_t bit_index = wiced_bt_ga_bap_get_sampling_freq_index(codec_arg->csc.sampling_frequency);
        if (!bit_index)
        {
            CAP_TRACE("Invalid sampling freq %d ", codec_arg->csc.sampling_frequency);
            return WICED_FALSE;
        }
        bit_index -= 1; // convert to supported sf LTV structures
        if (!(codec_param->sf & (1 << bit_index)))
        {
            CAP_TRACE("BAP_CODEC_CONFIG_SAMPLING_FREQUENCY_TYPE Error. ");
            return WICED_FALSE;
        }
    }

    if (codec_arg->csc.frame_duration)
    {
        // verify frame duration
        uint8_t bit_index = wiced_bt_ga_bap_get_fram_duration_index(codec_arg->csc.frame_duration);
        if (bit_index == 0xFF)
        {
            CAP_TRACE("Invalid frame duration %d ", codec_arg->csc.frame_duration);
            return WICED_FALSE;
        }
        if (!(codec_param->frame_duration & (1 << bit_index)))
        {
            CAP_TRACE("BAP_CODEC_CONFIG_FRAME_DURATION_TYPE Error. ");
            return WICED_FALSE;
        }
    }

    if (codec_arg->csc.audio_channel_allocation)
    {
        num_ch = ga_cap_get_num_bit_set(codec_arg->csc.audio_channel_allocation);
    }

    if (codec_arg->csc.lc3_blocks_per_sdu)
    {
        block_per_sdu = codec_arg->csc.lc3_blocks_per_sdu;
    }

    if (num_ch && block_per_sdu)
    {
        // check lc3 block per sdu & channel allocation
        if ((block_per_sdu * num_ch) < codec_param->frame_per_sdu) return WICED_FALSE;
    }

    return WICED_TRUE;
}
static wiced_bool_t ga_cap_veriry_audio_config_param(
    wiced_bt_ga_ascs_config_codec_args_t *p_codec_config,wiced_bt_ga_pacs_char_data_t *p_pac_list)
{
    le_audio_cap_codec_param_t record_param;
    int index = 0;

    for (index = 0; index < p_pac_list->num_of_records; index++)
    {
        wiced_bt_ga_pacs_record_t *record = &p_pac_list->record_list[index];
        if (record->codec_id.coding_format != p_codec_config->codec_id.coding_format)
        {
            CAP_TRACE("[%s] req_id %d peer_id %d  \n",
                      __FUNCTION__,
                      record->codec_id.coding_format,
                      p_codec_config->codec_id.coding_format);
            CAP_TRACE("Codec id didn't match ");
            return WICED_FALSE;
        }
        else
        {
            // go through the codec capabilities
            memset(&record_param, 0, sizeof(le_audio_cap_codec_param_t));
            ga_cap_fill_lc3_codec_param(record->codec_specific_capabilities,
                                        record->codec_specific_capabilities_length,
                                        &record_param);
            if (ga_cap_compare_codec_param(&record_param, p_codec_config))
            {
                break;
            }
        }
    }
    if (index >= p_pac_list->num_of_records) return WICED_FALSE;
    return WICED_TRUE;
}
wiced_bool_t le_audio_cap_verify_codec(uint8_t dir,
                                 wiced_bt_ga_ascs_config_codec_args_t *p_codec_config,
                                 wiced_bt_ga_pacs_data_t *pacs_data,
                                 uint8_t is_server)
{
    wiced_bt_ga_pacs_char_data_t *pac_list = NULL;

    if (dir & WICED_BT_CAP_DIRECTION_SOURCE)
    {
        pac_list = (is_server) ? &(pacs_data->source_pac_list) : &(pacs_data->sink_pac_list);
        if (!ga_cap_veriry_audio_config_param(p_codec_config, pac_list))
        {
            CAP_TRACE("[%s] dir %d", __FUNCTION__, WICED_BT_CAP_DIRECTION_SOURCE);
            return WICED_FALSE;
        }
    }
    if (dir & WICED_BT_CAP_DIRECTION_SINK)
    {
        pac_list = (is_server) ? &(pacs_data->sink_pac_list) : &(pacs_data->source_pac_list);
        if (!ga_cap_veriry_audio_config_param(p_codec_config, pac_list))
        {
            CAP_TRACE("[%s] dir %d", __FUNCTION__, WICED_BT_CAP_DIRECTION_SINK);
            return WICED_FALSE;
        }
    }

    // TODO: Preferred context type
    return WICED_TRUE;
}

void le_audio_cap_register_cb(le_audio_cap_cback_t *cb)
{
    if (cap_csip_data.p_app_cback == NULL)
    {
        cap_csip_data.p_app_cback = cb;
    }
}

static int ga_cap_get_curr_index(le_audio_cap_app_data_t *p_cap_app_data, uint16_t conn_id)
{
    for (int index = 0; index < p_cap_app_data->num_devices; index++)
    {
        if (p_cap_app_data->device_info_list[index].conn_id == conn_id)
        {
            return index;
        }
    }
    return -1;
}

void ga_cap_perform_op(le_audio_cap_app_data_t *p_cap_app_data,
                       le_audio_cap_start_unicast_param_t *p_unicast_params,
                       int curr_index,
                       wiced_bt_ga_ascs_opcode_t opcode)
{
    gatt_intf_attribute_t p_char = {0};
    static wiced_bt_ga_ascs_cp_cmd_t cmd_data;
    gatt_intf_service_object_t *ascs_profile = NULL;

    if (p_cap_app_data == NULL)
    {
        CAP_TRACE_CRIT("[%s] cap data is null\n", __FUNCTION__);
        return;
    }
    le_audio_cap_device_data_t *p_device_info = &p_cap_app_data->device_info_list[curr_index];
    int index;
    p_char.characteristic_type = ASCS_ASE_CONTROL_POINT_CHARACTERISTIC;

    if (p_device_info == NULL)
    {
        CAP_TRACE_CRIT("[%s] p_device info is null\n", __FUNCTION__);
        return;
    }

    ascs_profile = gatt_interface_get_service_by_uuid_and_conn_id(p_device_info->conn_id, &ga_service_uuid_ascs);

    if (!ascs_profile)
    {
        CAP_TRACE_CRIT("[%s] ascs profile is null\n", __FUNCTION__);
        return;
    }

    cmd_data.opcode = opcode;
    cmd_data.num_of_ase = p_device_info->num_ase;
    uint8_t param_size = sizeof(wiced_bt_ga_ascs_cp_params_t) * cmd_data.num_of_ase;
    cmd_data.p_cp_params = (wiced_bt_ga_ascs_cp_params_t *)wiced_bt_get_buffer(param_size);

    if (cmd_data.p_cp_params == NULL)
    {
        CAP_TRACE_CRIT("[%s] Memory allocation failed opcode %d \n", __FUNCTION__, opcode);
        return;
    }

    WICED_MEMSET(cmd_data.p_cp_params, 0, param_size);
    WICED_BT_TRACE("[%s] num ase : 0x%x opcode %d\n", __FUNCTION__, p_device_info->num_ase, opcode);
    for (index = 0; index < p_device_info->num_ase; index++)
    {
        wiced_bool_t is_start_ready = FALSE;
        wiced_bt_ga_ascs_cp_params_t *p_cp_params = &cmd_data.p_cp_params[index];
        wiced_bt_ga_ascs_ase_t *p_ase = p_device_info->ascs_data[index];
        p_cp_params->ase_id = p_ase->p_ase_info->ase_id;

        switch (opcode)
        {
        case WICED_BT_GA_ASCS_OPCODE_CONFIG_CODEC:
            p_cp_params->config_codec_params = *p_unicast_params->p_codec_configuration;
            p_cp_params->config_codec_params.csc.audio_channel_allocation =
                p_ase->codec_configured.csc.audio_channel_allocation;
            break;

        case WICED_BT_GA_ASCS_OPCODE_CONFIG_QOS:
        {
            p_cp_params->config_qos_params = *p_unicast_params->p_qos_configuration;
            p_cp_params->config_qos_params.cis_id = p_ase->qos_configured.cis_id;
        }
        break;

        case WICED_BT_GA_ASCS_OPCODE_ENABLE:
        case WICED_BT_GA_ASCS_OPCODE_UPDATE_METADATA:
            p_cp_params->metadata = p_unicast_params->metadata;
            break;

            //Send Receiver Start Ready/Receiver Stop Ready only for Source ASEs
        case WICED_BT_GA_ASCS_OPCODE_RECEIVER_START_READY:
        case WICED_BT_GA_ASCS_OPCODE_RECEIVER_STOP_READY:
            is_start_ready = (opcode == WICED_BT_GA_ASCS_OPCODE_RECEIVER_START_READY) ? TRUE : FALSE;
            if (p_ase->p_ase_info->ase_type == ASCS_SOURCE_ASE_CHARACTERISTIC)
                wiced_bt_ga_ascs_send_receiver_start_stop_ready(p_device_info->conn_id,
                                                                ascs_profile,
                                                                p_cp_params->ase_id,
                                                                is_start_ready);
            break;

        default:
            break;
        }
    }

    if (opcode != WICED_BT_GA_ASCS_OPCODE_RECEIVER_STOP_READY && opcode != WICED_BT_GA_ASCS_OPCODE_RECEIVER_START_READY)
        gatt_interface_write_characteristic(p_device_info->conn_id, ascs_profile, &p_char, &cmd_data);

    wiced_bt_free_buffer(cmd_data.p_cp_params);
}

void ga_cap_free_csip(void)
{
    if (cap_csip_data.p_csis_devices)
    {
        wiced_bt_free_buffer(cap_csip_data.p_csis_devices);
        cap_csip_data.p_csis_devices = NULL;
    }
}

void ga_cap_reset_state(le_audio_cap_app_data_t *p_cap_app_data)

{
    p_cap_app_data->is_disconnecting = FALSE;
    p_cap_app_data->is_disabling = FALSE;

    for (size_t i = 0; i < p_cap_app_data->num_devices; i++)
    {
        p_cap_app_data->device_info_list[i].ase_notification_count = 0;
    }

    //Safe check only
    ga_cap_free_csip();
}

wiced_bool_t le_audio_cap_fill_metadata(uint8_t ccid_count,
                                           uint8_t *ccid_list,
                                           wiced_bt_ga_bap_context_type_t context_type,
                                           wiced_bt_ga_bap_metadata_t *metadata)
{
    if (metadata)
    {
        if (metadata->preferred_audio_ctx & context_type)
            metadata->streaming_audio_ctx = context_type;
        if (ccid_list && ccid_count)
        {
            uint8_t *ptr = metadata->p_upper_layer_data;
            uint8_t length = (sizeof(uint8_t) * ccid_count) + 2;
            if (metadata->upper_layer_data_length < length) return WICED_FALSE;
            metadata->upper_layer_data_length = length;
            // TODO: Find CCID List metadata type from assigned numbers
            UINT8_TO_STREAM(ptr, ((sizeof(uint8_t) * ccid_count) + 1));
            UINT8_TO_STREAM(ptr, BAP_METADATA_CCID_LIST);
            WICED_MEMCPY(ptr, ccid_list, (sizeof(uint8_t) * ccid_count));
        }
        else
        {
            metadata->upper_layer_data_length = 0;
        }
    }
    return WICED_TRUE;
}

void ga_cap_send_state_change_evt(wiced_bt_ga_ascs_state_t state,
                                  int curr_index,
                                  le_audio_cap_app_data_t *p_app_data)
{
    le_audio_cap_event_data_t cap_event_data = {0};
    CAP_TRACE("[%s] state %d", __FUNCTION__, state);

    cap_event_data.group_state = state;
    cap_event_data.p_app_data = p_app_data;
    ga_cap_invoke_app_cb(p_app_data->device_info_list[curr_index].conn_id, WICED_BT_GA_CAP_STATE_CHANGED_EVENT, &cap_event_data);
}

void ga_cap_state_machine(le_audio_cap_app_data_t *p_cap_app_data,
                          le_audio_cap_start_unicast_param_t *p_unicast_params,
                          int curr_index,
                          wiced_bt_ga_ascs_ase_t *p_ase)
{
    uint8_t same_state = 0;
    uint8_t cap_stream_count;
    uint8_t cap_idle_count;

    CAP_TRACE("[%s] p_ase->ase_state %d ", __FUNCTION__, p_ase->ase_state);

    if (p_cap_app_data == NULL) return;

    if (p_ase->ase_state == WICED_BT_GA_ASCS_STATE_DISABLING || p_ase->ase_state == WICED_BT_GA_ASCS_STATE_RELEASING)
        return;

    /* Move to next command only after all the ASE's of the current device ack the current command */
    if (p_cap_app_data->device_info_list[curr_index].num_ase >= 1)
    {
        p_cap_app_data->device_info_list[curr_index].ase_notification_count++;

        if (p_cap_app_data->device_info_list[curr_index].ase_notification_count !=
            p_cap_app_data->device_info_list[curr_index].num_ase)
        {
            return;
        }
        else
            p_cap_app_data->device_info_list[curr_index].ase_notification_count = 0;
    }

    /* Move to next command only after all the connected devices ack the current command */
    if (p_cap_app_data->num_devices > 1)
    {
        /* Do not proceed on receiving Releasing notifications.
        Wait for Idle notification to move to next device */
        /* To enable second device, don't wait for streaming notification from first device  (Expected for TC : CAP/INI/UST/BV-01-C)*/
        //if ((WICED_BT_GA_ASCS_STATE_RELEASING != p_ase->ase_state))
            curr_index++;

        same_state = (curr_index != p_cap_app_data->num_devices) ? 1 : 0;

        if (curr_index == p_cap_app_data->num_devices) curr_index = 0;
    }

    switch (p_ase->ase_state)
    {
    case WICED_BT_GA_ASCS_STATE_IDLE:
        cap_idle_count = wiced_bt_ga_cap_utils_get_ascs_state_count(p_cap_app_data, WICED_BT_GA_ASCS_STATE_IDLE);

        if (cap_idle_count == p_cap_app_data->num_devices)
        {
            //Report WICED_BT_GA_ASCS_STATE_IDLE to the application
            ga_cap_send_state_change_evt(p_ase->ase_state, curr_index, p_cap_app_data);

            if (cap_start_csis_unlocking_procedure(p_cap_app_data, &cap_ascs_unlock_procedure_cmpl_cb) == WICED_FALSE)
            {
                ga_cap_reset_state(p_cap_app_data);
            }
        }
        else
        {
            ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_RELEASE);
        }
        break;
    case WICED_BT_GA_ASCS_STATE_CODEC_CONFIGURED:
        if (same_state)
        {
            if (p_cap_app_data->is_disconnecting)
            {
                ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_RELEASE);
            }
            else
            {
                ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_CONFIG_CODEC);
            }
        }
        else
        {
            //Report WICED_BT_GA_ASCS_STATE_CODEC_CONFIGURED to the application
            ga_cap_send_state_change_evt(p_ase->ase_state, curr_index, p_cap_app_data);

            curr_index = 0;
            if (p_cap_app_data->is_disconnecting)
            {
                cap_start_csis_unlocking_procedure(p_cap_app_data, &cap_ascs_unlock_procedure_cmpl_cb);
                ga_cap_reset_state(p_cap_app_data);
            }
            else
            {
                ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_CONFIG_QOS);
            }
        }
        break;
    case WICED_BT_GA_ASCS_STATE_QOS_CONFIGURED:
        if (same_state)
        {
            if (p_cap_app_data->is_disconnecting)
            {
                ga_cap_perform_op(p_cap_app_data,
                                  p_unicast_params,
                                  curr_index,
                                  WICED_BT_GA_ASCS_OPCODE_RECEIVER_STOP_READY);
            }
            else if (p_cap_app_data->is_disabling)
            {
                ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_DISABLE);
            }
            else
            {
                ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_CONFIG_QOS);
            }
        }
        else
        {
            //Report WICED_BT_GA_ASCS_STATE_QOS_CONFIGURED to the application
            ga_cap_send_state_change_evt(p_ase->ase_state, curr_index, p_cap_app_data);

            if (p_cap_app_data->is_disconnecting || p_cap_app_data->is_disabling)
            {
                cap_start_csis_unlocking_procedure(p_cap_app_data, &cap_ascs_unlock_procedure_cmpl_cb);
                ga_cap_reset_state(p_cap_app_data);
            }
            else
            {
                // QOS CONFIGURATION DONE FOR ALL THE DEVICES
                // Enable Streaming

                ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_ENABLE);
            }
        }
        break;
    case WICED_BT_GA_ASCS_STATE_ENABLING:
        if (same_state)
        {
            ga_cap_perform_op(p_cap_app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_ENABLE);
        }
        break;
    case WICED_BT_GA_ASCS_STATE_STREAMING:
        cap_stream_count = wiced_bt_ga_cap_utils_get_ascs_state_count(p_cap_app_data, WICED_BT_GA_ASCS_STATE_STREAMING);
        if (cap_stream_count == p_cap_app_data->num_devices)
        {
            cap_start_csis_unlocking_procedure(p_cap_app_data, &cap_ascs_unlock_procedure_cmpl_cb);
            //Report WICED_BT_GA_ASCS_STATE_STREAMING to the application
            ga_cap_send_state_change_evt(p_ase->ase_state, curr_index, p_cap_app_data);
        }
        break;
    default:
        break;
    }
}

wiced_bt_gatt_status_t cap_lock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                  void *data,
                                                  wiced_bt_gatt_status_t status)
{
    wiced_ga_cap_local_unicast_cb_data_t *cb_param = (wiced_ga_cap_local_unicast_cb_data_t *)data;
    CAP_TRACE("[%s] status %d", __FUNCTION__, status);
    if (status == WICED_SUCCESS)
    {
        ga_cap_perform_op(cb_param->app_data, cb_param->params, cb_param->curr_index, cb_param->opcode);
        wiced_bt_free_buffer(cb_param);
        return WICED_BT_SUCCESS;
    }

    CAP_TRACE_CRIT("[%s] LOCK FAILED ", __FUNCTION__);

    if (wiced_bt_ga_cap_utils_is_ascs_state(cb_param->app_data, WICED_BT_GA_ASCS_STATE_IDLE))
        ga_cap_reset_state(cb_param->app_data);

    // Free allocated buffer
    wiced_bt_free_buffer(cb_param);
    return WICED_BT_SUCCESS;
}

wiced_bt_gatt_status_t cap_ascs_unlock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                    void *data,
                                                    wiced_bt_gatt_status_t status)
{
    // Free allocated buffer
    ga_cap_free_csip();

    if (wiced_bt_ga_cap_utils_is_ascs_state((le_audio_cap_app_data_t *)data, WICED_BT_GA_ASCS_STATE_IDLE))
    {
        le_audio_cap_app_data_t *p_cap_app_data = (le_audio_cap_app_data_t *)data;
        ga_cap_reset_state(p_cap_app_data);
    }
    return WICED_BT_SUCCESS;
}

wiced_bt_ga_csip_set_entry *cap_csip_allocate_data(le_audio_cap_app_data_t *app_data)
{
    int index;
    // Currently allocated by some other API
    if (cap_csip_data.p_csis_devices != NULL)
    {
        return NULL;
    }

    cap_csip_data.p_csis_devices =
        (wiced_bt_ga_csip_set_entry *)wiced_bt_get_buffer(sizeof(wiced_bt_ga_csip_set_entry) * app_data->num_devices);

    if (cap_csip_data.p_csis_devices == NULL) return NULL;

    for (index = 0; index < app_data->num_devices; index++)
    {
        uint16_t conn_id = app_data->device_info_list[index].conn_id;
        cap_csip_data.p_csis_devices[index].conn_id = conn_id;
        cap_csip_data.p_csis_devices[index].p_service =
            gatt_interface_get_service_by_uuid_and_conn_id(conn_id, &ga_service_uuid_csis);
        if (cap_csip_data.p_csis_devices[index].p_service == NULL)
        {
            ga_cap_free_csip();
            return NULL;
        }
    }
    return cap_csip_data.p_csis_devices;
}

wiced_bool_t cap_start_csis_locking_procedure(le_audio_cap_app_data_t *app_data,
                                              wiced_bt_lock_procedure_cmpl_cb callback,
                                              void *data)
{
    if (cap_csip_data.enable_locking == WICED_FALSE)
    {
        if(callback)callback(NULL, data, WICED_SUCCESS);
        return WICED_TRUE;
    }
    if (cap_csip_allocate_data(app_data) == NULL) return WICED_FALSE;

    if (app_data->is_bonded)
    {
        // start locking procedure
        wiced_bt_ga_csip_start_lock_procedure(cap_csip_data.p_csis_devices,
                                              app_data->num_devices,
                                              WICED_BT_GA_CSIS_LOCKED,
                                              callback,
                                              data);
    }
    else
    {
        // use order access procedure
        wiced_bt_ga_csip_start_ordered_access_procedure(cap_csip_data.p_csis_devices,
                                                        app_data->num_devices,
                                                        callback,
                                                        data);
    }
    return WICED_TRUE;
}

wiced_bool_t cap_start_csis_unlocking_procedure(le_audio_cap_app_data_t *p_cap_app_data,
                                                wiced_bt_lock_procedure_cmpl_cb callback)
{
    wiced_bool_t status = WICED_FALSE;
    // If data is not alloated then allocate & fill it
    if (cap_csip_data.enable_locking == WICED_FALSE)
    {
        if (callback) callback(NULL, p_cap_app_data, WICED_SUCCESS);
        return WICED_TRUE;
    }
    if (cap_csip_data.p_csis_devices == NULL)
    {
        cap_csip_allocate_data(p_cap_app_data);
    }
    if (cap_csip_data.p_csis_devices)
    {
        if (p_cap_app_data->is_bonded)
        {
            //unlock all the devices
            if (wiced_bt_ga_csip_start_lock_procedure(cap_csip_data.p_csis_devices,
                                                      p_cap_app_data->num_devices,
                                                      WICED_BT_GA_CSIS_UNLOCKED,
                                                      callback,
                                                      p_cap_app_data) == WICED_SUCCESS)
            {
                status = WICED_TRUE;
            }
        }
        ga_cap_free_csip();
    }
    return status;
}

void *cap_allocate_unicast_cb_data(le_audio_cap_app_data_t *app_data,
                                   le_audio_cap_start_unicast_param_t *params,
                                   int curr_index,
                                   uint8_t opcode)
{
    wiced_ga_cap_local_unicast_cb_data_t *data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_unicast_cb_data_t));
    if (data)
    {
        data->app_data = app_data;
        data->curr_index = curr_index;
        data->opcode = opcode;
        data->params = params;
    }
    return (void *)data;
}

uint8_t le_audio_cap_mutiplex_audio_supported(wiced_bt_ga_ascs_config_codec_args_t *p_codec_param,
                                              wiced_bt_ga_pacs_data_t *p_pacs_data)
{
    for (int i = 0; i < p_pacs_data->sink_pac_list.num_of_records; i++)
    {
        wiced_bt_ga_pacs_record_t *record = &p_pacs_data->sink_pac_list.record_list[i];
        le_audio_cap_codec_param_t record_param;
        memset(&record_param, 0, sizeof(le_audio_cap_codec_param_t));
        ga_cap_fill_lc3_codec_param(record->codec_specific_capabilities,
                                    record->codec_specific_capabilities_length,
                                    &record_param);
       if (ga_cap_compare_codec_param(&record_param, p_codec_param))
       {
           if (record_param.audio_ch_count > 1)
           {
               return WICED_TRUE;
           }
           return WICED_FALSE;
       }
    }
    return WICED_FALSE;
}

wiced_result_t le_audio_cap_start_unicast_streaming(le_audio_cap_app_data_t *app_data,
                                                    le_audio_cap_start_unicast_param_t *params)
{
    uint8_t index;
    uint16_t requested_context;
    void *cb_data;

    CAP_TRACE("[%s] ", __FUNCTION__);

    // verify all data pointers
    if ((app_data == NULL) || (params == NULL) || (app_data->num_devices == 0) ||
        (app_data->device_info_list == NULL) || (params->p_codec_configuration == NULL))
    {
        CAP_TRACE_CRIT("[%s] bad arg\n", __FUNCTION__);
        return WICED_BT_BADARG;
    }
    requested_context = params->context_type;

    // verify context type is available or not
    for (index = 0; index < app_data->num_devices; index++)
    {
        wiced_bt_ga_pacs_data_t *pacs_data = app_data->device_info_list[index].pacs_data;

        if (pacs_data == NULL)
        {
            CAP_TRACE_CRIT("[%s] PACS data is null\n", __FUNCTION__);
            return WICED_BT_BADARG;
        }

        // verify context type
        if (ga_cap_verify_context_type(params->dir,
                                       requested_context,
                                       &(pacs_data->supported),
                                       &(pacs_data->available)) == WICED_FALSE)
        {
            CAP_TRACE_CRIT("[%s] Bad context type\n", __FUNCTION__);
            return WICED_BT_BADARG;
        }
        // verify audio location
#if 0
        if (ga_cap_verify_audio_location(params->dir, params->location, pacs_data) == WICED_FALSE)
        {
            CAP_TRACE_CRIT("[%s] Bad audio location\n", __FUNCTION__);
            return WICED_BT_BADARG;
        }
#endif
    }

    CAP_TRACE("[%s] Configurations verified ", __FUNCTION__);

    ga_cap_reset_state(app_data);

    // CSIS Present
    gatt_intf_service_object_t *csis_profile =
        gatt_interface_get_service_by_uuid_and_conn_id(app_data->device_info_list[0].conn_id, &ga_service_uuid_csis);

    if (csis_profile)
    {
        cb_data = cap_allocate_unicast_cb_data(app_data, params, 0, WICED_BT_GA_ASCS_OPCODE_CONFIG_CODEC);
        if (cb_data == NULL) return WICED_BT_ERROR;

        if (!cap_start_csis_locking_procedure(app_data, &cap_lock_procedure_cmpl_cb, cb_data))
        {
            WICED_BT_TRACE("[%s] Lock Failed", __FUNCTION__);
            return WICED_BT_ERROR;
        }
    }
    else
    {
        // Configure Codec
        ga_cap_perform_op(app_data, params, 0, WICED_BT_GA_ASCS_OPCODE_CONFIG_CODEC);
    }

    return WICED_SUCCESS;
}

wiced_result_t le_audio_cap_ascs_update_event(le_audio_cap_app_data_t *app_data,
                                              le_audio_cap_start_unicast_param_t *p_unicast_params,
                                              uint16_t conn_id,
                                              wiced_bt_gatt_status_t status,
                                              uint32_t evt_type,
                                              gatt_intf_attribute_t *p_char,
                                              void *p_data,
                                              int len)
{

    if ((evt_type == NOTIFICATION_EVT) && !wiced_bt_ga_csip_is_lock_procedure_in_progress())
    {
        int curr_index = ga_cap_get_curr_index(app_data, conn_id);
        if (curr_index == -1)
        {
            CAP_TRACE_CRIT("[%s] conn_id 0x%x not found ", __FUNCTION__, conn_id);
            return WICED_ERROR;
        }

        if (p_char->characteristic_type == ASCS_ASE_CONTROL_POINT_CHARACTERISTIC)
        {
            // TODO: Handle error

            // For update metadata command, state wont be changed. So handle it in control point success
            wiced_bt_ga_ascs_cp_notif_t *cmd_data = (wiced_bt_ga_ascs_cp_notif_t *)p_data;
            if ((cmd_data->opcode == WICED_BT_GA_ASCS_OPCODE_UPDATE_METADATA) &&
                (cmd_data->p_status->response_code == WICED_BT_GA_ASCS_RESPONSE_SUCCESS))
            {
                curr_index++;
                if (curr_index != app_data->num_devices)
                {
                    // same state
                    ga_cap_perform_op(app_data, p_unicast_params, curr_index, WICED_BT_GA_ASCS_OPCODE_UPDATE_METADATA);
                }
                else
                {
                    // Metadata Updated for all the devices
                    cap_start_csis_unlocking_procedure(app_data, &cap_ascs_unlock_procedure_cmpl_cb);
                }
            }
        }
        if (p_char->characteristic_type == ASCS_SINK_ASE_CHARACTERISTIC ||
            p_char->characteristic_type == ASCS_SOURCE_ASE_CHARACTERISTIC)
        {
            ga_cap_state_machine(app_data, p_unicast_params, curr_index, p_data);
        }
    }
    return WICED_BT_SUCCESS;
}

wiced_result_t le_audio_cap_update_streaming(le_audio_cap_app_data_t *app_data,
                                                le_audio_cap_start_unicast_param_t *unicast_param,
                                                wiced_bt_ga_bap_context_type_t context_type,
                                                wiced_bt_ga_bap_metadata_t *metadata)
{
    void *cb_data;
    if (wiced_bt_ga_cap_utils_is_ascs_state(app_data, WICED_BT_GA_ASCS_STATE_STREAMING))
    {
        uint8_t index = 0;
        // verify context type is available or not
        for (index = 0; index < app_data->num_devices; index++)
        {
            wiced_bt_ga_pacs_data_t *pacs_data = app_data->device_info_list[index].pacs_data;
            // verify context type
            if (ga_cap_verify_context_type(unicast_param->dir,
                                           context_type,
                                           &(pacs_data->supported),
                                           &(pacs_data->available)) == WICED_FALSE)
            {
                return WICED_BT_BADARG;
            }
        }
        cb_data = cap_allocate_unicast_cb_data(app_data, unicast_param, 0, WICED_BT_GA_ASCS_OPCODE_UPDATE_METADATA);
        if ((cb_data == NULL) ||
            cap_start_csis_locking_procedure(app_data, &cap_lock_procedure_cmpl_cb, cb_data) == WICED_FALSE)
        {
            ga_cap_perform_op(app_data, unicast_param, 0, WICED_BT_GA_ASCS_OPCODE_UPDATE_METADATA);
        }
        return WICED_SUCCESS;
    }
    else
    {
        CAP_TRACE_CRIT("[%s] current state is not streaming\n", __FUNCTION__);
    }
    return WICED_ERROR;
}

wiced_result_t le_audio_cap_disable_stream(le_audio_cap_app_data_t *app_data)
{
    ga_cap_reset_state(app_data);
    void *cb_data = cap_allocate_unicast_cb_data(app_data, NULL, 0, WICED_BT_GA_ASCS_OPCODE_DISABLE);
    app_data->is_disabling = TRUE;
    if ((cb_data == NULL) || cap_start_csis_locking_procedure(app_data, &cap_lock_procedure_cmpl_cb, cb_data) == WICED_FALSE)
    {
        ga_cap_perform_op(app_data, NULL, 0, WICED_BT_GA_ASCS_OPCODE_DISABLE);
    }
    return WICED_SUCCESS;
}

wiced_result_t le_audio_cap_release_stream(le_audio_cap_app_data_t *app_data)
{
    ga_cap_reset_state(app_data);
    void *cb_data = cap_allocate_unicast_cb_data(app_data, NULL, 0, WICED_BT_GA_ASCS_OPCODE_RELEASE);
    app_data->is_disconnecting = TRUE;

    if ((cb_data == NULL) ||
        cap_start_csis_locking_procedure(app_data, &cap_lock_procedure_cmpl_cb, cb_data) == WICED_FALSE)
    {
        ga_cap_perform_op(app_data, NULL, 0, WICED_BT_GA_ASCS_OPCODE_RELEASE);
    }
    return WICED_SUCCESS;
}

wiced_result_t le_audio_cap_vcs_update_event(le_audio_cap_app_data_t *app_data,
                                             uint16_t conn_id,
                                             wiced_bt_gatt_status_t status,
                                             uint32_t evt_type,
                                             gatt_intf_attribute_t *p_char,
                                             void *p_data,
                                             int len)
{
    if ((status == WICED_BT_GATT_SUCCESS) && (evt_type == NOTIFICATION_EVT) &&
        !wiced_bt_ga_csip_is_lock_procedure_in_progress())
    {
        if ((p_char->included_service_type == INCLUDED_SERVICE_NONE) &&
            (p_char->characteristic_type == VCS_VOLUME_STATE_CHARACTERISTIC))
        {
            // ABS volume or Output Mute state
            int i;
            wiced_bt_ga_vcs_data_t *vcs_data = (wiced_bt_ga_vcs_data_t *)p_data;
            gatt_intf_attribute_t p_char_local = {.characteristic_type = VCS_CONTROL_POINT_CHARACTERISTIC};
            wiced_bt_ga_vcs_data_t data;

            for (i = 0; i < app_data->num_devices; i++)
            {
                if (app_data->device_info_list[i].conn_id != conn_id)
                {
                    if (app_data->device_info_list[i].vcs_data->volume_setting !=
                        vcs_data->control_point_data.volume_state.volume_setting)
                    {
                        // update volume
                        gatt_intf_service_object_t *p_service =
                            gatt_interface_get_service_by_uuid_and_conn_id(conn_id, &ga_service_uuid_vcs);

                        data.control_point_data.opcode = VOLUME_CONTROL_OPCODE_SET_ABSOLUTE_VOLUME;
                        data.control_point_data.volume_state.volume_setting =
                            vcs_data->control_point_data.volume_state.volume_setting;

                        if (p_service)
                            gatt_interface_write_characteristic(app_data->device_info_list[i].conn_id,
                                                                p_service,
                                                                &p_char_local,
                                                                &data);
                    }
                    if (app_data->device_info_list[i].vcs_data->mute_state !=
                        vcs_data->control_point_data.volume_state.mute_state)
                    {
                        // Update Mute state
                        uint16_t conn_id_local = app_data->device_info_list[i].conn_id;
                        gatt_intf_service_object_t *p_service =
                            gatt_interface_get_service_by_uuid_and_conn_id(conn_id_local, &ga_service_uuid_vcs);

                        if (vcs_data->control_point_data.volume_state.mute_state == WICED_BT_MUTE_STATE_MUTED)
                            data.control_point_data.opcode = VOLUME_CONTROL_OPCODE_MUTE;
                        else if (vcs_data->control_point_data.volume_state.mute_state == WICED_BT_MUTE_STATE_NOT_MUTED)
                            data.control_point_data.opcode = VOLUME_CONTROL_OPCODE_UNMUTE;
                        if (p_service)
                            gatt_interface_write_characteristic(conn_id_local, p_service, &p_char_local, &data);
                    }
                }
            }
        }
    }
    return WICED_SUCCESS;
}

wiced_result_t le_audio_cap_mics_update_event(le_audio_cap_app_data_t *app_data,
                                                 uint16_t conn_id,
                                                 wiced_bt_gatt_status_t status,
                                                 uint32_t evt_type,
                                                 gatt_intf_attribute_t *p_char,
                                                 void *p_data,
                                                 int len)
{
    if ((status == WICED_BT_GATT_SUCCESS) && (evt_type == NOTIFICATION_EVT) &&
        !wiced_bt_ga_csip_is_lock_procedure_in_progress())
    {
        if ((p_char->included_service_type == INCLUDED_SERVICE_NONE) &&
            (p_char->characteristic_type == MICS_MUTE_STATE_CHARACTERISTIC))
        {
            int i;
            gatt_intf_attribute_t p_char_local = {.characteristic_type = MICS_MUTE_STATE_CHARACTERISTIC,
                                                  .included_service_type = INCLUDED_SERVICE_NONE};
            wiced_bt_ga_mics_data_t data;
            wiced_bt_ga_mics_data_t *mics_data = (wiced_bt_ga_mics_data_t *)p_data;
            data.mute_val = mics_data->mute_val;

            //Update mute state
            for (i = 0; i < app_data->num_devices; i++)
            {
                if ((app_data->device_info_list[i].conn_id != conn_id) &&
                    (mics_data->mute_val != (app_data->device_info_list[i].mics_mute_val)))
                {
                    uint16_t conn_id_local = app_data->device_info_list[i].conn_id;
                    gatt_intf_service_object_t *p_service =
                        gatt_interface_get_service_by_uuid_and_conn_id(conn_id_local, &ga_service_uuid_mics);
                    if (p_service) gatt_interface_write_characteristic(conn_id_local, p_service, &p_char_local, &data);
                }
            }
        }
    }
    return WICED_SUCCESS;
}

static wiced_result_t ga_cap_send_vcs_command(wiced_ga_cap_local_volume_data_t *volume_data)
{
    int i;

    for (i = 0; i < volume_data->app_data->num_devices; i++)
    {
        uint16_t conn_id = volume_data->app_data->device_info_list[i].conn_id;
        gatt_intf_service_object_t *p_service =
            gatt_interface_get_service_by_uuid_and_conn_id(conn_id, &ga_service_uuid_vcs);

        if (p_service == NULL)
        {
            return WICED_BT_SERVICE_NOT_FOUND;
        }
        gatt_interface_write_characteristic(conn_id,
                                            p_service,
                                            &volume_data->volume_characteristic,
                                            &volume_data->vcs_data);
    }
    return WICED_SUCCESS;
}

static wiced_result_t ga_cap_send_vcs_command_helper(wiced_ga_cap_local_volume_data_t *volume_data)
{
    wiced_result_t result = WICED_SUCCESS;

    if (cap_start_csis_locking_procedure(volume_data->app_data, &cap_volume_lock_procedure_cmpl_cb, volume_data) ==
        WICED_FALSE)
    {
        result = ga_cap_send_vcs_command(volume_data);
        wiced_bt_free_buffer(volume_data);
        ga_cap_free_csip();
    }
    return result;
}

wiced_bt_gatt_status_t cap_unlock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                           void *data,
                                                           wiced_bt_gatt_status_t status)
{
    ga_cap_free_csip();
    return WICED_SUCCESS;
}

wiced_bt_gatt_status_t cap_volume_lock_procedure_cmpl_cb(gatt_intf_service_object_t *p_csip_inst,
                                                         void *data,
                                                         wiced_bt_gatt_status_t status)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = (wiced_ga_cap_local_volume_data_t *)data;
    CAP_TRACE("[%s] status %d", __FUNCTION__, status);

    // Cleanup allocated CSIP data
    ga_cap_free_csip();

    if (status == WICED_SUCCESS)
    {
        if (vcs_data->volume_characteristic.characteristic_type == VCS_CONTROL_POINT_CHARACTERISTIC)
            ga_cap_send_vcs_command(vcs_data);
        else
            ga_cap_send_mics_command(vcs_data);

        if (cap_start_csis_unlocking_procedure(vcs_data->app_data, &cap_unlock_procedure_cmpl_cb) == WICED_FALSE)
        {
            ga_cap_free_csip();
        }
        wiced_bt_free_buffer(vcs_data);
        return WICED_BT_SUCCESS;
    }
    wiced_bt_free_buffer(vcs_data);
    CAP_TRACE_CRIT("[%s] LOCK FAILED ", __FUNCTION__);
    cap_unlock_procedure_cmpl_cb(NULL, NULL, WICED_SUCCESS);

    return WICED_BT_SUCCESS;
}

wiced_result_t le_audio_cap_set_volume(le_audio_cap_app_data_t *app_data, volume_control_opcodes_t opcode, uint8_t abs_vol)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_volume_data_t));
    if (vcs_data == NULL)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    WICED_MEMSET(vcs_data, 0, sizeof(wiced_ga_cap_local_volume_data_t));
    vcs_data->app_data = app_data;
    vcs_data->vcs_data.control_point_data.opcode = opcode;
    vcs_data->vcs_data.control_point_data.volume_state.volume_setting = abs_vol;
    vcs_data->volume_characteristic.characteristic_type = VCS_CONTROL_POINT_CHARACTERISTIC;

    return ga_cap_send_vcs_command_helper(vcs_data);
}

wiced_result_t le_audio_cap_set_volume_mute_state(le_audio_cap_app_data_t *app_data,
                                                     wiced_bt_ga_mute_val_t mute_state)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_volume_data_t));
    if (vcs_data == NULL)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    WICED_MEMSET(vcs_data, 0, sizeof(wiced_ga_cap_local_volume_data_t));
    vcs_data->app_data = app_data;

    if (mute_state == WICED_BT_MUTE_STATE_MUTED)
        vcs_data->vcs_data.control_point_data.opcode = VOLUME_CONTROL_OPCODE_MUTE;
    else if (mute_state == WICED_BT_MUTE_STATE_NOT_MUTED)
        vcs_data->vcs_data.control_point_data.opcode = VOLUME_CONTROL_OPCODE_UNMUTE;

    vcs_data->volume_characteristic.characteristic_type = VCS_CONTROL_POINT_CHARACTERISTIC;

    return ga_cap_send_vcs_command_helper(vcs_data);
}

wiced_result_t le_audio_cap_set_volume_offset(le_audio_cap_app_data_t *app_data, int16_t volume_offset)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_volume_data_t));
    if (vcs_data == NULL)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    WICED_MEMSET(vcs_data, 0, sizeof(wiced_ga_cap_local_volume_data_t));
    vcs_data->app_data = app_data;

    vcs_data->vocs_data.volume_offset = volume_offset;
    vcs_data->vcs_data.volume_included.p_vocs = &vcs_data->vocs_data;

    vcs_data->volume_characteristic.included_service_type = INCLUDED_SERVICE_VCS_VOCS;
    vcs_data->volume_characteristic.characteristic_type = VOCS_VOLUME_OFFSET_CONTROL_POINT_CHARACTERISTIC; // TBD

    return ga_cap_send_vcs_command_helper(vcs_data);
}

static wiced_result_t ga_cap_send_mics_command(wiced_ga_cap_local_volume_data_t *volume_data)
{
    int i;

    for (i = 0; i < volume_data->app_data->num_devices; i++)
    {
        uint16_t conn_id = volume_data->app_data->device_info_list[i].conn_id;
        gatt_intf_service_object_t *p_service =
            gatt_interface_get_service_by_uuid_and_conn_id(conn_id, &ga_service_uuid_mics);
        if (p_service == NULL)
        {
            WICED_BT_TRACE("[%s] service not found", __FUNCTION__);
            return WICED_BT_SERVICE_NOT_FOUND;
        }
        gatt_interface_write_characteristic(conn_id,
                                            p_service,
                                            &volume_data->volume_characteristic,
                                            &volume_data->mics_data);
    }
    return WICED_SUCCESS;
}

static wiced_result_t ga_cap_send_mics_command_helper(wiced_ga_cap_local_volume_data_t *volume_data)
{
    wiced_result_t result = WICED_SUCCESS;
    if (cap_start_csis_locking_procedure(volume_data->app_data, &cap_volume_lock_procedure_cmpl_cb, volume_data) ==
        WICED_FALSE)
    {
        result = ga_cap_send_mics_command(volume_data);
        ga_cap_free_csip();
        wiced_bt_free_buffer(volume_data);
    }
    return result;
}

wiced_result_t le_audio_cap_set_mics_mute_state(le_audio_cap_app_data_t *app_data,
                                                   wiced_bt_ga_mute_val_t mute_state)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_volume_data_t));
    if (vcs_data == NULL)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    WICED_MEMSET(vcs_data, 0, sizeof(wiced_ga_cap_local_volume_data_t));
    vcs_data->app_data = app_data;

    vcs_data->mics_data.mute_val = mute_state;
    vcs_data->volume_characteristic.characteristic_type = MICS_MUTE_STATE_CHARACTERISTIC;

    return ga_cap_send_mics_command_helper(vcs_data);
}

wiced_result_t le_audio_cap_set_mics_aics_mute_state(le_audio_cap_app_data_t *app_data,
                                                     uint32_t instance,
                                                     wiced_bt_ga_mute_val_t mute_state)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_volume_data_t));
    if (vcs_data == NULL)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    WICED_MEMSET(vcs_data, 0, sizeof(wiced_ga_cap_local_volume_data_t));
    vcs_data->app_data = app_data;

    vcs_data->aics_data.control_point.opcode =
        mute_state ? WICED_BT_GA_AICS_OPCODE_SET_MUTE : WICED_BT_GA_AICS_OPCODE_SET_UNMUTE;
    vcs_data->mics_data.mics_included.p_aics = &vcs_data->aics_data;
    vcs_data->volume_characteristic.characteristic_type = AICS_INPUT_CONTROL_POINT_CHARACTERISTIC;
    vcs_data->volume_characteristic.included_service_type = INCLUDED_SERVICE_MICS_AICS;
    vcs_data->volume_characteristic.included_service_instance = instance;
    vcs_data->app_data = app_data;

    return ga_cap_send_mics_command_helper(vcs_data);
}

wiced_result_t le_audio_cap_set_mics_aics_gain(le_audio_cap_app_data_t *app_data, uint32_t instance, int8_t gain)
{
    wiced_ga_cap_local_volume_data_t *vcs_data = wiced_bt_get_buffer(sizeof(wiced_ga_cap_local_volume_data_t));
    if (vcs_data == NULL)
        return WICED_BT_OUT_OF_HEAP_SPACE;

    WICED_MEMSET(vcs_data, 0, sizeof(wiced_ga_cap_local_volume_data_t));
    vcs_data->aics_data.control_point.opcode = WICED_BT_GA_AICS_OPCODE_SET_GAIN_SETTINGS;
    vcs_data->aics_data.control_point.input_state.gain_setting = gain;
    vcs_data->mics_data.mics_included.p_aics = &vcs_data->aics_data;

    vcs_data->volume_characteristic.characteristic_type = AICS_INPUT_CONTROL_POINT_CHARACTERISTIC;
    vcs_data->volume_characteristic.included_service_type = INCLUDED_SERVICE_MICS_AICS;
    vcs_data->volume_characteristic.included_service_instance = instance;
    vcs_data->app_data = app_data;

    return ga_cap_send_mics_command_helper(vcs_data);
}

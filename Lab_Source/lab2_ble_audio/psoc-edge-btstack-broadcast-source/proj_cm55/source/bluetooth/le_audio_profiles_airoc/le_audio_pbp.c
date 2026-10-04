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

#include "wiced_data_types.h"
#include "le_audio_pbp.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_isoc.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_ble.h"

#define PUBLIC_BROADCAST_ANNOUNCEMENT_SIZE 5

wiced_bool_t le_audio_pbp_is_public_broadcast(uint16_t adv_len, uint8_t *p_adv_data,
                                              le_audio_rcv_public_broadcast_t *p_public_rcv_br)
{
    uint8_t *p_data = NULL;
    uint16_t uuid = 0;
    uint16_t adv_entry_length = 0;
    uint8_t public_broadcast_feature = 0;
    uint8_t br_name_size = 0;

    wiced_bt_adv_ctx_t ctx = {
       .adv_len = adv_len,
       .p_adv = p_adv_data
    };
    memset(p_public_rcv_br->broadcast_name, 0, MAX_BROADCAST_NAME_ADV_SIZE);
    p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, & adv_entry_length);
    while (adv_entry_length)
    {
        STREAM_TO_UINT16(uuid, p_data);
        if (uuid == WICED_BT_UUID_PUBLIC_BROADCAST_ANNOUNCEMENT)
        {
            PBP_BROADCAST_TRACE("[%s] uuid : %d", __FUNCTION__, uuid);
            STREAM_TO_UINT8(public_broadcast_feature, p_data);
            PBP_BROADCAST_TRACE("[%s] public broadcast feature: %d", __FUNCTION__, public_broadcast_feature);
            p_public_rcv_br->public_broadcast_feature = public_broadcast_feature;
            if (public_broadcast_feature & 1)
                p_public_rcv_br->encryption = WICED_BLE_ISOC_ENCRYPTED;
            else
                p_public_rcv_br->encryption = WICED_BLE_ISOC_UNENCRYPTED;
            if (public_broadcast_feature & 2)
                p_public_rcv_br->audio_config = WICED_BT_GA_PBP_STANDARD_QUALITY_PUBLIC_BROADCAST_AUDIO;
            else if (public_broadcast_feature & 4)
                p_public_rcv_br->audio_config = WICED_BT_GA_PBP_HIGH_QUALITY_PUBLIC_BROADCAST_AUDIO;
            else
                p_public_rcv_br->audio_config = WICED_BT_GA_PBP_GENERIC_CODEC_PUBLIC_BROADCAST_AUDIO;

            STREAM_TO_UINT8(p_public_rcv_br->metadata_length, p_data);
            if (p_public_rcv_br->metadata_length > 0)
            STREAM_TO_ARRAY(p_public_rcv_br->metadata, p_data, p_public_rcv_br->metadata_length);
            break;
        }
        else
        {
            p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, & adv_entry_length);
            if (adv_entry_length == 0) return FALSE;
        }
    }

    /* change adv search type */
    ctx.offset = 0;
    p_data = wiced_ble_adv_data_search(&ctx, WICED_BT_GA_PBP_TYPE_BROADCAST_NAME, & adv_entry_length);
    if (p_data == NULL || adv_entry_length == 0)
    {
        return FALSE;
    }
    br_name_size =
        (adv_entry_length < MAX_BROADCAST_NAME_ADV_SIZE) ? adv_entry_length : MAX_BROADCAST_NAME_ADV_SIZE - 1;
    STREAM_TO_ARRAY(p_public_rcv_br->broadcast_name, p_data, br_name_size);
    p_public_rcv_br->broadcast_name[br_name_size] = '\0';
    PBP_BROADCAST_TRACE("[%s] name : %s", __FUNCTION__, p_public_rcv_br->broadcast_name);

    /* change adv search type */
    ctx.offset = 0;
    p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_APPEARANCE, & adv_entry_length);
    if (p_data == NULL || adv_entry_length == 0)
    {
        PBP_BROADCAST_TRACE_CRIT("no appearance info");
        return TRUE;
    }
    STREAM_TO_UINT16(p_public_rcv_br->source_appearance_value, p_data);
    return TRUE;
}

wiced_result_t le_audio_pbp_build_adv_data(uint8_t *p_data,
                                              uint32_t offset,
                                              le_audio_public_broadcast_t *p_public_broadcast,
                                              uint32_t* len)
{
    PBP_BROADCAST_TRACE("[%s]", __FUNCTION__);

    if (p_data == NULL || p_public_broadcast == NULL) return WICED_ERROR;

    uint8_t *p_adv_data = p_data + offset;
    uint8_t public_audio_announcement_feature = 0;

    if (p_public_broadcast->program_info_size > MAX_PROGRAM_INFO_SIZE)
        p_public_broadcast->program_info_size = MAX_PROGRAM_INFO_SIZE;

    if (p_public_broadcast->metadata_length > MAX_METADATA_LEN)
        p_public_broadcast->metadata_length = MAX_METADATA_LEN;

    if (p_public_broadcast->name_length > MAX_BROADCAST_NAME_ADV_SIZE)
        p_public_broadcast->name_length = MAX_BROADCAST_NAME_ADV_SIZE;

    // WICED_BT_UUID_PUBLIC_BROADCAST_ANNOUNCEMENT data
    UINT8_TO_STREAM(p_adv_data,
                    PUBLIC_BROADCAST_ANNOUNCEMENT_SIZE + p_public_broadcast->program_info_size + p_public_broadcast->metadata_length + 2);
    UINT8_TO_STREAM(p_adv_data, BTM_BLE_ADVERT_TYPE_SERVICE_DATA);
    UINT16_TO_STREAM(p_adv_data, WICED_BT_UUID_PUBLIC_BROADCAST_ANNOUNCEMENT);
    if (p_public_broadcast->encryption)
        public_audio_announcement_feature |= WICED_BLE_ISOC_ENCRYPTED;
    else
        public_audio_announcement_feature |= WICED_BLE_ISOC_UNENCRYPTED;
    if (p_public_broadcast->sampling_frequency == 48000)
        public_audio_announcement_feature |= WICED_BT_GA_PBP_HIGH_QUALITY_PUBLIC_BROADCAST_AUDIO;
    else if ((p_public_broadcast->sampling_frequency == 16000 || p_public_broadcast->sampling_frequency == 24000) && p_public_broadcast->frame_duration == 10000)
        public_audio_announcement_feature |= WICED_BT_GA_PBP_STANDARD_QUALITY_PUBLIC_BROADCAST_AUDIO;
    UINT8_TO_STREAM(p_adv_data, public_audio_announcement_feature);

     UINT8_TO_STREAM(p_adv_data,
                    p_public_broadcast->metadata_length + p_public_broadcast->program_info_size + 2); // Length of metadata

     //--Metadata--(L-T-V structure)
     UINT8_TO_STREAM(p_adv_data, p_public_broadcast->program_info_size + 1); // Program info
     UINT8_TO_STREAM(p_adv_data, PBP_METADATA_PROGRAM_INFO_TYPE);
     if (p_public_broadcast->program_info_size > 0)
     {
         if (p_public_broadcast->program_info == NULL) return WICED_ERROR;
         ARRAY_TO_STREAM(p_adv_data, p_public_broadcast->program_info, p_public_broadcast->program_info_size);
     }

    //--Metadata--
     if (p_public_broadcast->metadata_length > 0)
     {
         if (p_public_broadcast->metadata == NULL) return WICED_ERROR;
         ARRAY_TO_STREAM(p_adv_data, p_public_broadcast->metadata, p_public_broadcast->metadata_length);
     }

    //BROADCAST NAME DATA AD Type
    if (p_public_broadcast->name_length == 0 || p_public_broadcast->broadcast_name == NULL)
    {
        return WICED_ERROR;
    }
    UINT8_TO_STREAM(p_adv_data, p_public_broadcast->name_length + 1);
    UINT8_TO_STREAM(p_adv_data, WICED_BT_GA_PBP_TYPE_BROADCAST_NAME);
    ARRAY_TO_STREAM(p_adv_data, p_public_broadcast->broadcast_name, p_public_broadcast->name_length);

    //APPEARANCE VALUE AD Type
    UINT8_TO_STREAM(p_adv_data, APPEARANCE_VALUE_AD_SIZE);
    UINT8_TO_STREAM(p_adv_data, BTM_BLE_ADVERT_TYPE_APPEARANCE);
    UINT16_TO_STREAM(p_adv_data, p_public_broadcast->appearance_value);//appearance value should be taken from app

    *len = p_adv_data - (p_data + offset);
    return WICED_SUCCESS;
}

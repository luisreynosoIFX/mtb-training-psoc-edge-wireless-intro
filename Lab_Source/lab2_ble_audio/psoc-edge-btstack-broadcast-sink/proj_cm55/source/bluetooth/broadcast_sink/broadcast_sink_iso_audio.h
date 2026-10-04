/******************************************************************************
* File Name        : broadcast_sink_iso_audio.h
*
* Description      : This file declares the ISO audio data path APIs used by
*                    the broadcast sink application.
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

#pragma once

#include "wiced_bt_isoc.h"
#include "wiced_bt_ga_bap.h"

void iso_audio_init(const wiced_bt_cfg_isoc_t *p_iso_cfg);

wiced_result_t iso_audio_setup_data_path(uint16_t conn_id,
                                         uint16_t direction,
                                         wiced_bt_ga_bap_csc_t *p_csc,
                                         uint8_t bis_count);

void iso_audio_remove_data_path(uint16_t conn_hdl, wiced_ble_isoc_data_path_bit_t dir, uint8_t *idx_list);

void iso_audio_start_stream(uint16_t conn_id);

void iso_audio_stop_stream(uint16_t conn_id);

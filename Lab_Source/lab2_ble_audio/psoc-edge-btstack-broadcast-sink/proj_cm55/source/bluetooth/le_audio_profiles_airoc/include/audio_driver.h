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

#ifndef __AUDIO_DRIVER_H__
#define __AUDIO_DRIVER_H__

#include "wiced_bt_isoc.h"
#include <stdio.h>

void audio_driver_init(wiced_ble_isoc_data_path_bit_t dir, uint8_t num_of_channels, uint32_t sample_rate);

void audio_driver_deinit(uint8_t direction);

void audio_driver_write_non_interleaved_data(uint8_t *p_left_data,
                                             uint8_t *p_right_data,
                                             uint8_t bit_width_in_bytes,
                                             uint32_t data_size);

void audio_driver_set_volume(uint8_t volume);
void audio_driver_set_mute_state(uint8_t mute_enabled);
void audio_driver_load_wave_file(wiced_ble_isoc_data_path_bit_t dir, uint32_t sample_rate);
void audio_driver_mic_init(uint8_t num_of_channels, uint32_t sample_rate);
void audio_driver_set_mic_gain(int32_t gain);
void audio_driver_set_mic_mute_state(uint8_t mute, int32_t gain);

#endif //__AUDIO_DRIVER_H__

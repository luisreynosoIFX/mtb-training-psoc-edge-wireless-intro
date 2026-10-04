#pragma once

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_LC3_SAMPLE_RATE 48000U
#define AUDIO_LC3_FRAME_US 10000U
#define AUDIO_LC3_FRAME_BYTES 100U

bool audio_lc3_format_supported(uint32_t sample_rate, uint32_t frame_us,
                               uint16_t frame_bytes, uint8_t channels);
bool audio_lc3_copy_frame(uint32_t *frame_index, uint8_t channels,
                         uint8_t *destination, uint16_t capacity);
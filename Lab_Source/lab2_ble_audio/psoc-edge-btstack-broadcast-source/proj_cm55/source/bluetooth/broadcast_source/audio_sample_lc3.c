#if !ENABLE_LC3_ENCODING
#include <string.h>

#include "audio_sample_lc3.h"
#include "audio_sample_lc3.inc"

bool audio_lc3_format_supported(uint32_t sample_rate, uint32_t frame_us,
                               uint16_t frame_bytes, uint8_t channels)
{
    return sample_rate == AUDIO_LC3_SAMPLE_RATE && frame_us == AUDIO_LC3_FRAME_US &&
           frame_bytes == AUDIO_LC3_FRAME_BYTES && (channels == 1 || channels == 2);
}

bool audio_lc3_copy_frame(uint32_t *frame_index, uint8_t channels,
                         uint8_t *destination, uint16_t capacity)
{
    if (!frame_index || !destination || (channels != 1 && channels != 2) ||
        capacity < AUDIO_LC3_FRAME_BYTES * channels)
        return false;

    uint32_t current = *frame_index % AUDIO_LC3_FRAME_COUNT;
    memcpy(destination, audio_lc3_frames[current], AUDIO_LC3_FRAME_BYTES);
    if (channels == 2)
        memcpy(destination + AUDIO_LC3_FRAME_BYTES, audio_lc3_frames[current], AUDIO_LC3_FRAME_BYTES);
    *frame_index = (current + 1) % AUDIO_LC3_FRAME_COUNT;
    return true;
}
#endif
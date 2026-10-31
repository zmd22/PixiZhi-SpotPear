#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome::usb_audio {
// Signed 16-bit PCM attenuation; equal gain for L/R, no channel remapping.
// Input remains untouched, including on a retry after USB back-pressure.
inline void attenuate_s16(uint8_t *dst, const uint8_t *src, size_t bytes, uint32_t gain_q15) {
  for (size_t i = 0; i + sizeof(int16_t) <= bytes; i += sizeof(int16_t)) {
    int16_t sample;
    std::memcpy(&sample, src + i, sizeof(sample));
    const int16_t scaled = static_cast<int16_t>((static_cast<int32_t>(sample) * static_cast<int32_t>(gain_q15)) / 32768);
    std::memcpy(dst + i, &scaled, sizeof(scaled));
  }
}
}  // namespace esphome::usb_audio

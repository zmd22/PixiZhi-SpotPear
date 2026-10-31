# PixiZhi SpotPear USB software volume patch — 0.9.8

Based on ESPHome 2026.11.0-dev usb_audio sources, retrieved 2026-10-31.
Upstream: https://github.com/esphome/esphome/tree/dev/esphome/components/usb_audio
License: see LICENSE. Runtime sources are GPLv3; Python is MIT.

Local changes on 2026-10-31:
- Optional `software_volume: true` on the USB speaker, restricted to 16-bit PCM.
- Thread-safe Q15 PCM attenuation and software mute, without changing channels,
  audio frame reporting, USB packet accounting, or the output router.
- In software mode, skip all USB feature-unit volume/mute requests, including
  the initial requests on stream start. The DAC retains its hardware level.
- Process bounded 512-byte chunks without modifying caller data or allocating
  memory during playback. Retries use original samples.
- Default false preserves upstream hardware-volume behavior.

Install the entire components/usb_audio folder next to pixizhi.yaml.
Do not combine this snapshot with partial usb_audio source updates.

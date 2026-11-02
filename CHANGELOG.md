# Changelog

## 0.9.9

- Kitchen timer counts from an absolute end time (no drift), survives reboots
  (running timers resume after time sync; expired ones ring), and exposes
  `Timer Ends` (timestamp) and `Timer State` entities to Home Assistant.
- Notifications persist across reboots and show their age ("12M AGO").
- Immediate HA/Wi-Fi offline indication on clock and dashboards; commands fail
  fast with "HA OFFLINE" instead of waiting for the 7 s confirmation timeout.
- Battery: readings are held during playback (speaker sag), new
  `Charging (estimated)` sensor from the idle voltage trend, configurable
  `Low Battery Warning` shown on the clock.
- Power: no frame work while the standby screen is black; optional
  `Deep Standby After` (off by default) with touch wake on the CST816 interrupt.
- Release check against GitHub (`Latest Release`, `Firmware Update Available`,
  `Check for Update`). TLS verification is now enabled for HTTP requests.
- Pizza screen supports up to three pages of four actions (swipe left/right);
  the screens blueprint adds a second page (slices 5–8).
- UI state constants and pure helpers moved to `pixizhi_ui.h`.

## 0.9.8

Initial public source release.

- Pixel-only clock and menus for home controls, notifications, timer and actions.
- Larger category/status text and vertical category navigation.
- Scheduled red night styling with independent saved brightness.
- Three Home Assistant blueprints, with no preselected household entity IDs.
- Internal/USB output selection, UAC 1.0 audio and USB software volume/mute.
- Paired-server Sendspin playback and on-device dynamic pairing code.
- Generic configuration, separate credential template, original font/sounds,
  preview images, installation guidance and third-party notices.

Requires ESPHome 2026.11.0-dev. Full UI/touch and sustained-playback verification
on physical hardware remains an early-release testing task.

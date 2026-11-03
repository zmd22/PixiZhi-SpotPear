# Changelog

## 1.1.0

- New "Soft LED" look on every screen: glowing round LEDs, one 5x7 font (no tiny
  3x5 text), unlit LED grid behind the clock digits, redrawn 9x9 and 11x11 icons.
- 20 fully saturated theme colours (`Theme Color`); status colours (battery,
  alarms) stay fixed, night mode still turns everything red.
- Quick settings: swipe down on the clock. Battery, Wi-Fi, HA link and time,
  plus brightness, ball volume, night mode, Do Not Disturb, audio output, timer.
- Do Not Disturb: normal notifications no longer light the LED and HA LED
  flashes are skipped; urgent alerts, timer and siren still sound.
- Sendspin album artwork on the player screen, as LED dots or the full image
  (`Artwork Style`).
- What's that plane: with `Plane Spotting` on, every new plane from the
  What's-that-plane integration is shown for 15 s (several planes cycle),
  with a takeoff icon for departures from `Home Airport` and a landing icon
  otherwise. Screens blueprint gains a "What's that plane" section.
- All drawing moved to `pixizhi_draw.h`; `tools/render_previews.cpp` renders
  the docs images with the same code.

## 1.0.0

- Public, credential-free firmware: Wi-Fi via the `PixiZhi Setup` hotspot with an
  on-screen QR code, API encryption key provisioned by Home Assistant, timezone
  received from Home Assistant. `secrets.yaml` is no longer needed.
- On-screen onboarding: setup QR, then IP and "add to Home Assistant" hint.
- Firmware updates from GitHub releases through the Home Assistant update entity
  (HTTPS with certificate verification, MD5-checked image). Network-push OTA is
  removed from the public build.
- Release workflow builds and attaches images and publishes a browser installer.
- Battery ring around the clock edge (`Battery Ring` switch), hidden while charging.
- `dashboard_import` for ESPHome "Take control".
- Release check entities from 0.9.9 replaced by the native update entity.
- Update screen with progress, "Restarting", or "FAILED / Old firmware kept".
- Automatic rollback: a new image must run for two minutes before it is
  confirmed; a crash or reset before then restores the previous firmware.
- Release notes (release title and text) shown in Home Assistant's update dialog.
- Siren: full-screen STOP button that works without Home Assistant, replays the
  built-in sound reliably without relying on media repeat, keeps the original
  volume when switched on twice, and hands the LED back to notifications.
- Siren always plays on the built-in speaker at full volume, even with the USB
  DAC selected; volume and mute are locked while it sounds, and the previous
  output and volume are restored afterwards.

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

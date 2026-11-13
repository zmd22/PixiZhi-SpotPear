# PixiZhi SpotPear

**1.1.4 · ESPHome firmware and Home Assistant blueprints for the Spotpear Ball v2**

A bold pixel interface for a small round desktop display: clock, home controls,
music, thermostats, status, notifications, timer, and four quick actions.
Configure your entities in Home Assistant rather than hardcoding them in firmware.

![Pixel interface examples](docs/images/interface-overview.png)

*Firmware drawing-code previews with fictional sample entities. Night images show
red pixel colours; the physical brightness setting is not simulated.*

## Features

- Pixel clock with weather/status rotation, temperature/humidity, and optional
  playing-track information.
- Up to six favourite light, switch, media-player, or thermostat controls.
  Larger category lists scroll vertically and show a V hint when more are below.
- Touch controls and rim gestures for brightness, volume, and temperature.
- Red night interface on every page, scheduled start/end times, and an independent
  saved Night Mode Brightness. Schedules crossing midnight are supported.
- Pixel notifications, an adjustable kitchen timer, and a radial menu of up to
  eight Home Assistant actions (long-press the clock).
- Sendspin playback with paired-server authentication and a six-digit pairing
  screen. Existing trust is stored across restarts.
- Internal ES8311 speaker or USB UAC 1.0 DAC output, manual selection, and optional
  fallback to the internal speaker.
- USB software volume/mute to avoid sending hardware volume controls that can
  stall some DACs. Native stereo is preserved; mono sounds are duplicated to L/R.
- Three included Home Assistant blueprints: screens/controls, display notifications,
  and notification-LED flashes.

## Requirements

- **Spotpear Ball v2 hardware matching this pinout:** ESP32-S3, 16 MB flash,
  8 MB octal PSRAM, 240×240 GC9A01A display, CST816 touch, ES8311 audio.
- **ESPHome 2026.11.0-dev with ESP-IDF.** This configuration depends on development
  components and is not intended for older stable ESPHome releases.
- Home Assistant with its ESPHome integration. Enable **Allow the device to
  perform Home Assistant actions** for the configured device.
- A compatible Sendspin server for local music playback. The current ESPHome
  implementation uses Sendspin 1.0.0-rc1 (for example Music Assistant 2.11.0b4+).

The ESP32-S3 does not support Bluetooth Classic/A2DP audio reception. This firmware
is not a Bluetooth music receiver, and BLE proxy functionality is disabled.
USB audio requires a compatible **UAC 1.0** device and a working host power/data
connection. A higher-power DAC or a different board may need additional hardware.

## Install

**First install (once, over USB):**

1. Open the installer page — `https://zmd22.github.io/PixiZhi-SpotPear/` — in Chrome
   or Edge on a computer.
2. Hold **BOOT** while plugging in the ball (the firmware uses USB for audio, so the
   chip must start in download mode), click **Install**, pick the port.
3. Reconnect normally. The ball shows **PIXIZHI SETUP** and a QR code. Scan it, or
   join the `PixiZhi Setup` Wi-Fi, and choose your network.
4. The ball then shows its IP and "add to Home Assistant". In Home Assistant open
   **Settings → Devices & services**, add the discovered **PixiZhi SpotPear** and
   enable **Allow the device to perform Home Assistant actions**. Home Assistant
   creates a unique API encryption key for the device during this step.
5. Import the blueprints by URL (buttons below) and create an automation from each
   one you need, with **ESPHome device name** `pixizhi`:
   - [Screens and controls](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fzmd22%2FPixiZhi-SpotPear%2Fmain%2Fblueprints%2Fpixizhi_screens_blueprint.yaml)
   - [Display notifications](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fzmd22%2FPixiZhi-SpotPear%2Fmain%2Fblueprints%2Fpixizhi_notify_blueprint.yaml)
   - [LED notifications](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fzmd22%2FPixiZhi-SpotPear%2Fmain%2Fblueprints%2Fpixizhi_led_notifications.yaml)
   Imported this way, a blueprint updates with **Settings → Automations & scenes →
   Blueprints → ⋮ → Re-import blueprint**; your automations keep their settings.

**Updates:** new releases appear as a **Firmware** update in Home Assistant.
Click Install; the ball downloads the image from GitHub over verified HTTPS and
checks its MD5 before flashing. Wi-Fi, the API key and all settings are kept.

The public firmware contains no credentials and no network-push OTA. To build
your own variant instead, use ESPHome's **Take control** on the discovered device
(or copy `pixizhi.yaml`, `pixizhi_ui.h`, `pixizhi_draw.h`, `components/`, `fonts/`, `sounds/`) and add
your own `ota:`/`wifi:` settings; see [configuration notes](docs/CONFIGURATION.md).

## Use

- Tap the clock for the four-category launcher. A single configured light/player
  opens its controls directly; multiple entities open a list.
- Swipe up/down through category lists, or tap V for more. The home icon returns
  to the clock. On regular control pages, centre swipes navigate or go back.
- Turn the outer rim on the clock for brightness, on music controls for volume,
  and on thermostats for target temperature. Keep navigation swipes in the centre.
- Player: tap the small previous / play-pause / next controls, swipe left/right
  in the lower half, or double-tap to play/pause. Tap the artwork for options.
- Lights: the palette button (or a swipe up) opens the colour picker; tap a
  colour or one of the three whites, swipe down to go back.
- Notifications: swipe vertically through a long message, tap to dismiss a normal
  alert, or hold for one second to dismiss an urgent alert.
- Timer: tap −/+ to set minutes, then START. PAUSE/RESUME controls playback of the
  countdown; hold the main button to reset. Home closes the page, not the timer.
- Radial menu: hold the middle of the clock until the ring appears, slide to an
  action and let go. Releasing in the middle keeps it open for a tap; it closes
  after 8 s. Actions 1-4 sit top/right/bottom/left, 5-8 on the diagonals.
- Enable **Night Mode** to use its schedule. Start is inclusive, end is exclusive;
  equal times disable the interval. **Night Mode Brightness** defaults to 10%.
  The clock rim adjusts that value at night; daytime brightness is retained.

See [configuration](docs/CONFIGURATION.md), [audio and pairing](docs/AUDIO.md),
and [validation notes](docs/VALIDATION.md).

## Development

Releases are built by GitHub Actions against a pinned ESPHome 2026.11.0-dev
commit. Every push also runs the host tests (`tests/test_helpers.cpp`), the
public-source check and an ESPHome config validation. The screenshots are
produced by the firmware's drawing code: `python3 tools/build_docs_images.py`.
Problems and ideas are welcome in the repository's issues.

## Credits and licensing

Hardware definitions are based on
[RealDeco/xiaozhi-esphome](https://github.com/RealDeco/xiaozhi-esphome).
The project builds on [ESPHome](https://github.com/esphome/esphome), LVGL,
ESP-IDF, and Sendspin. The bundled USB component is an ESPHome development
snapshot with a local software-volume patch; see its [patch notes](components/usb_audio/PATCH.md).

Project-authored configuration, blueprints, documentation, pixel font, and
synthesized sounds are GPLv3-licensed. Bundled third-party files retain their
licenses, including **GPLv3 for the ESPHome USB C++ runtime** and MIT for its
Python code. See [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY.md).

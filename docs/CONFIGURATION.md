# Configuration

## Public defaults

The public device name is `pixizhi` and the friendly name is PixiZhi SpotPear.
The firmware contains no credentials: Wi-Fi is entered through the setup hotspot
and stored on the device; Home Assistant provisions the API encryption key when
the device is added. Home Assistant 2026.3+ sends its timezone to the device, so
schedules follow your HA timezone (UTC is only the fallback for older HA).

The firmware contains no Home Assistant entity IDs. The screens blueprint owns
favourite controls, standby climate sensors, standby player, weather, status,
quick actions, and optional gesture actions. Select those entities when creating
your automation. Temperature, humidity, player, and weather selections are
required inputs; they have no preselected household defaults.

The six favourites may be lights, switches, media players, or thermostats.
A light group counts as a configured entity; the firmware does not automatically
discover or count every light in a home. Thermostats use reported limits and
steps; the control supports a single target temperature, not separate heating
and cooling setpoints.

## Blueprints

- `blueprints/pixizhi_screens_blueprint.yaml`: favourite screens, standby data,
  status slots, four quick actions, gesture actions, and maximum dial volume.
- `blueprints/pixizhi_notify_blueprint.yaml`: normal and urgent display alerts.
- `blueprints/pixizhi_led_notifications.yaml`: temporary LED flashes with restore.

All three default to device name `pixizhi`. Their display names are rebranded;
the public firmware and blueprints use matching `esphome.pixizhi_*` events.
Older custom configurations using different event names should keep their
matching blueprints or migrate both sides together.

For a custom ESPHome name containing dashes, the blueprint's service-name input
uses underscores. Event payload matching in the screens blueprint accepts the
firmware name's dashes as well; do not put a friendly display name there.

Status slots can match state, attributes, both, or numeric thresholds. Give each
slot an icon, label, priority, optional displayed value, and tap destination or
action. Unavailable entities are not treated as active matches.

## Night mode

Night Mode enables scheduled styling; it is not a permanent manual red-mode
switch. Default times are 22:00 and 07:00. The start time is included and the end
excluded. Crossing midnight is supported; equal endpoints mean no active range.
When Home Assistant time is not valid yet, the daytime settings apply.

Night Mode Brightness is an independent saved 1–100% setting, initially 10%.
Screen wake and urgent notifications respect it. Daytime brightness and standby
dimming return when the schedule ends. The clock rim adjusts the night value
while the schedule is active. All display colours become red/black, including
picker and pairing controls. The separate notification LED keeps its configured
colours/effects.

## Gestures and feedback

Clock long press is a stationary 1.4-second hold, configurable as Timer or an HA
action. Rim adjustment consumes the touch so it does not also start a timer or
issue a swipe/tap. The first touch after dimming wakes the screen only.

HA command feedback waits for state confirmation. A timeout shows
NO CONFIRMATION; it does not prove the command was rejected. Unavailable/stale
controls are blocked. Clock, brightness, and timer remain local.

The optional Pause Display During Audio setting defaults off. It is a diagnostic
switch: the screen may show old information while paused. Leave it off for
normal use, and use HA controls during a comparison test.

## Power and battery

- **Deep Standby After** (minutes, 0 = off): after the screen has dimmed for this
  long, the device deep-sleeps and wakes on a touch. While asleep it is offline:
  no HA control, music, notifications or LED alerts. It never sleeps while
  `Charging (estimated)` is on, during playback, a running/finished timer, an
  urgent alert, the siren, or Sendspin pairing.
- **Charging (estimated)**: the board exposes no charger signal, so charging is
  inferred from the idle battery voltage (≥ 4.12 V, or rising over two minutes).
  Expect a delay of a minute or two after plugging in or out.
- Battery readings are frozen while audio plays, because speaker load lowers the
  measured voltage.
- With **Standby Brightness** at 0 %, the standby screen is not redrawn until woken.

## Re-adding a device

If Home Assistant asks for an encryption key for a ball it knew before, delete
the old ESPHome entry and restart Home Assistant; it then provisions a new key
without asking. A first-time device is never asked for a key.

## Battery ring

**Battery Ring** (on by default) draws 60 dots around the clock's outer edge,
clockwise from 12 o'clock: green above 50 %, amber above 20 %, red below. It is
hidden while `Charging (estimated)` is on and is red-tinted in night mode.

## Setup screen

Until Wi-Fi is configured, the ball runs the open `PixiZhi Setup` hotspot and shows
a QR code for it. After Wi-Fi connects, it shows its IP and an "add to Home
Assistant" hint until Home Assistant connects for the first time (tap LATER to
hide it until the next restart). A configured device whose router is down shows
"WIFI OFFLINE" on the clock instead. The hotspot only appears after two minutes
without a connection, and it is open; anyone nearby can use it to change the
saved network during that window.

## Updates

The **Firmware** update entity reads
`https://zmd22.github.io/PixiZhi-SpotPear/manifest.json` (`update_manifest_url`)
after boot and every six hours. Publishing a GitHub release whose tag matches the version in
`pixizhi.yaml` (for example `v1.0.0`) runs `.github/workflows/release.yml`, which
builds the firmware, attaches the OTA/factory images and manifest, and publishes
the browser installer to GitHub Pages (enable Pages with source **GitHub Actions**
once in the repository settings). Pre-releases are built and attached, but
devices only follow the latest full release.

While an update installs, the screen shows its progress; media and the siren
stop first. A new image has to run for two minutes before it is confirmed. If it
crashes or resets before that, the bootloader returns to the previous firmware
on its own. Write release notes in the GitHub release text: its first ~250
characters appear in Home Assistant's update dialog.

A release is a public build: anything in `pixizhi.yaml` is visible to everyone.
Never add credentials to it.

## Siren

The **Siren** switch plays the built-in `sounds/alarm.wav` at full volume on a loop,
pulses the LED red and shows a full-screen **STOP** button. Everything runs on the
device, so STOP works even if Home Assistant or the network is down. It stops on
its own after **Siren Max Duration** (default 120 s); afterwards the previous
volume is restored and the LED returns to its notification colour, or switches off.

## Theme

**Theme Color** picks one of 20 saturated accent colours for values, icons and
rings. Text stays white/grey and status colours stay fixed (green battery and
connection, red alarms). **Artwork Style** chooses how Sendspin album art is
shown on the player screen: LED dots (default) or the full image. Artwork is
shown when the player screen's current title matches the track the ball is
playing through Sendspin.

## Quick settings

Swipe down on the clock. Tiles (tap to change): brightness (25/50/75/100 %;
night brightness while night mode is active), ball volume (steps of 20 %),
night mode, Do Not Disturb, audio output (speaker/USB DAC) and the timer.
Swipe up to close.

## What's that plane

Requires the What's-that-plane custom integration. In the screens blueprint,
check the **Visible flights sensor** (default `sensor.visible_flights`). On the
device, turn on **Plane Spotting** and set **Home Airport** (IATA code, default
`FRA`). Each plane that newly enters the view is shown for 15 seconds; several
new planes are shown one after another, then the ball returns to the previous
screen. Departures from the home airport get a takeoff icon, all others a
landing icon. Tap or swipe to dismiss. The integration provides no flight
number, so the callsign is shown as the flight code and the registration on
the bottom line.

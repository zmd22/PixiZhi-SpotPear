# Configuration

## Public defaults

The public device name is `pixizhi`, the friendly name is PixiZhi SpotPear, and
the timezone is `Etc/UTC`. All credentials come from an ignored `secrets.yaml`.
Set the timezone substitution to your local IANA timezone before using schedules.

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

## Power and battery (0.9.9)

- **Deep Standby After** (minutes, 0 = off): after the screen has dimmed for this
  long, the device deep-sleeps and wakes on a touch. While asleep it is offline:
  no HA control, music, notifications or LED alerts. It never sleeps while
  `Charging (estimated)` is on, during playback, a running/finished timer, an
  urgent alert, the siren, or Sendspin pairing.
- **Charging (estimated)**: the board exposes no charger signal, so charging is
  inferred from the idle battery voltage (≥ 4.17 V, or rising over five minutes).
  Expect a delay of a few minutes after plugging in or out.
- Battery readings are frozen while audio plays, because speaker load lowers the
  measured voltage.
- With **Standby Brightness** at 0 %, the standby screen is not redrawn until woken.

## Updates

The firmware checks `https://api.github.com/repos/<github_repo>/releases/latest`
two minutes after boot and every 12 hours (`github_repo` substitution). It only
reports a newer release; you still rebuild with your own secrets to install it.

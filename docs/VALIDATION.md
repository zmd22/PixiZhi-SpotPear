# Validation and scope

## Checked on every push

- Host tests (`tests/test_helpers.cpp`): notification storage round-trips and
  overflow, age labels, timer arithmetic, version comparison, text folding
  (accents, quotes, dashes, unsupported scripts), colour-picker hit-testing, and
  a render of every screen in day and night mode that checks for out-of-bounds
  writes and that night frames contain no green or blue.
- `tools/check_public.py`: no credentials, addresses or build artefacts.
- `esphome config` against the pinned ESPHome commit.

Releases are compiled by the release workflow with the same pinned commit.

## Checked by hand in earlier releases

Night schedule ranges (crossing midnight, exact boundaries, equal endpoints,
invalid time) and the USB software-volume helper over every signed 16-bit sample
at seven gains were verified for 0.9.8 and have not changed since.

## Not covered

The preview images use fictional sample data. They are firmware canvas renders,
not photographs or a simulation of physical PWM brightness. Automated checks do
not exercise touch paths on hardware, network conditions, deep standby wake, or
individual USB DACs; those are confirmed on a physical ball before a release
where the change touches them.

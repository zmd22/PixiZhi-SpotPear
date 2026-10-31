# Validation and scope

The 0.9.8 UI was compiled with ESPHome 2026.11.0-dev and ESP-IDF.
Host-side checks exercised the actual schedule/backlight lambdas and the actual
pixel-raster functions used by the firmware.

Checks covered every second of five schedule ranges, crossing midnight, exact
boundaries, equal endpoints, disabled mode, invalid time, independent night
brightness and daytime restoration. Night clock/heating/timer images were
checked for zero green and blue components. Blueprint API action names and
notification icon coverage were checked before packaging.

The USB gain helper was tested over every signed 16-bit input sample at seven
gains, with distinct stereo values, preserved source data, and retry stability.
The software-volume workaround was confirmed responsive on compatible hardware.

The preview images use fictional sample data. They are firmware canvas renders,
not photographs or a simulation of physical PWM brightness. They do not establish
that every touch path, native LVGL picker, network condition, or USB device has
been verified physically. The complete 0.9.8 UI and authentication flow still
need on-device verification of those behaviours.

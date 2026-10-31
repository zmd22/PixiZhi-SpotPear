# Audio and Sendspin

## Outputs

Audio Output selects Internal speaker or USB DAC. The internal amplifier stays
off in USB mode. USB Audio Fallback defaults on: when audio is pending but USB
does not start for three seconds, the selector returns to Internal speaker.
Idle USB selection does not trigger fallback, and the firmware does not switch
back automatically.

The pipeline normalizes media and announcements to 48 kHz, 16-bit, two-channel
PCM. Mono sounds are duplicated to left and right; stereo streams retain their
separate channels. A format conversion stage cannot fix an absent USB interface,
unsupported UAC mode, or missing power/data connection.

The local Audio Output Test plays the included chime at 20% and interrupts local
playback. Test the internal output first, then USB. Start at low volume.

## Software USB volume

Some USB DACs can stall audio when the host sends hardware volume or mute
requests. The bundled component exposes `software_volume: true`, which scales
signed 16-bit PCM in bounded chunks and suppresses those USB control requests,
including initial stream-start controls. It does not change packet accounting,
frame callbacks, channel order, or the output router.

At software volume 100%, samples pass unchanged; this is attenuation, not a
boost. The DAC retains its own hardware gain/volume. If maximum volume is low,
check the DAC's gain, EQ, and hardware level with its supported configuration
utility. Keep UAC 1.0 selected afterward. Internal-speaker volume retains its
existing hardware path.

PCM at 48 kHz/16-bit stereo is a useful starting point on reliable Wi-Fi: it
avoids compressed-audio decoding and carries about 1.54 Mbit/s before overhead.
FLAC uses less bandwidth with lossless decoding work; Opus uses less bandwidth
with lossy decoding. A stalled output is not necessarily a codec problem.

## Authentication

Sendspin connections are encrypted. `unpaired_access: false` additionally
restricts playback to paired servers. Discovery and pairing requests are still
possible; the setting is not a network firewall.

If the server requests pairing, the device displays a fresh six-digit code.
Enter it on the server. An active code is also exposed as Sendspin Pairing Code
in Home Assistant. Existing trust is retained in flash across normal reboots.
A server that used unpaired access previously must pair before it can play.

HIDE dismisses the panel only. After too many wrong codes, ALLOW or the HA button
Sendspin Allow Pairing Retry lets an attempt continue without bypassing code
verification. Dynamic-code pairing does not normally require that confirmation.
There is no general unpaired-access bypass switch in this configuration.

## Maintenance

Keep the entire bundled USB component together with this configuration. It is a
snapshot based on ESPHome 2026.11.0-dev, not a promise of compatibility with every
later development build. Test internal/USB routing, volume, mute, announcements,
and sustained playback after updating ESPHome. Do not partially replace sources.

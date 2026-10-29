# Bridge75 Fn Battery Gauge

QMK firmware that turns F1–F12 into a battery gauge while Fn is held. Releasing
Fn turns the lights off. Built for the Bridge75 ANSI RGB / Max, using battery
reports from the wireless module.

## Behavior

- 50% lights F1–F6, 75% lights F1–F9, and 100% lights all twelve keys.
- The gauge runs red through green, with a proportional final segment.
- 0% flashes F1 red. Missing or stale readings show dim purple.
- Both Windows and Mac Fn layers work. Other keys stay dark.
- Key layouts, wireless controls, and VIA support are retained.

## Implementation

The UART parser checks packet checksums and rejects percentages above 100.
Readings expire after 15 seconds and are invalidated on connection changes or
sleep/wake. Battery queries also run over USB.

The lighting hook clears each RGB render chunk before drawing the gauge.
Temporary RGB settings keep saved effects from bleeding through without
writing to EEPROM. Suspend and wireless low-power states override Fn lighting.

This repository is an overlay for
[emolitor/qmk_firmware](https://github.com/emolitor/qmk_firmware), based on
`em-2.0.0` at `1153ac2b39e937a2c2f6bd1e956643518a2117e3`.
`keyboards/` contains the modified firmware; `battery-project/` contains the
native test harness; `firmware/` contains the compiled binary and manifest.

## Build

Requires QMK build dependencies, the `qmk` CLI, and an Arm bare-metal GNU
toolchain. The committed binary was built with Arm GNU 14.2.Rel1 / GCC 14.2.1.

```sh
sh scripts/build.sh
```

The script checks out the pinned upstream revision under `.build/qmk`, applies
the overlay, and compiles `shortcut/bridge75/ansi:battery`. Set `ARM_PREFIX` if
the compiler is outside the default path:

```sh
ARM_PREFIX=/path/to/bin/arm-none-eabi- sh scripts/build.sh
```

Output: `.build/qmk/shortcut_bridge75_ansi_battery.bin`.
The build script does not flash the keyboard.

## Tests

Requires Clang:

```sh
sh battery-project/test.sh
```

The tests compile the UART parser and lighting hooks against hardware stubs.
They cover all 101 battery levels, malformed packets, fragmented input, timer
rollover, stale readings, Fn release, suspend/wake, and RGB render chunks.
Undefined-behavior and bounds sanitizers are enabled.

## Binary

The [committed binary](firmware/bridge75-ansi-fn-battery.bin) is 49,996 bytes.
Verify its checksum from the repository root:

```sh
(cd firmware && shasum -a 256 -c SHA256SUMS)
```

Build and native tests have passed; this binary has not been tested on the
keyboard. Build identity and checksum are recorded in
[firmware/manifest.json](firmware/manifest.json).

GPL-2.0-or-later. Upstream copyright and license notices are retained.
See [LICENSE](LICENSE).
